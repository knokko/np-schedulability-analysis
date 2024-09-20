# Automatic reconfiguration, the first prototype
## The flow
I added a `--reconfigure` argument to the CLI. When present, the tool will call
`NP::Reconfiguration::Manager::run_with_automatic_reconfiguration(const Options options, Scheduling_problem<Time> problem)`.
The `run_with_automatic_reconfiguration` will check whether the given problem is schedulable. If it's already schedulable,
it will just print `The given problem is already schedulable` and quit.

If it's not schedulable, it will try a number of *reconfiguration strategies* to find potential adaptations that would make
the problem schedulable. As soon as any strategy finds a solution, it will be printed to stdout, after which it quits. An
example output is given below:
```
The given problem is not schedulable, but you can make it schedulable by following these steps:
Increase the best-case running time of the job with ID T1J1 from 1 to the worst-case running time 2
Increase the best-case running time of the job with ID T2J7 from 7 to the worst-case running time 8
```
When a strategy does not manage to find a solution, the program will attempt the next strategy. When all strategies fail,
it will print `The given problem is not schedulable, and I couldn't find a solution to fix it`, and quit.

## The reconfiguration agent
All strategies work by repeatedly checking whether the problem is schedulable (each time with a slight variation of the
problem), using `NP::Global::State_space<Time>::explore`. In many iterations, they need to extract information during
the exploration, and possibly manipulate its execution. Since the `State_space` class is long and complicated enough
as-is, I wanted to push these extra strategy manipulations and data collection into different classes. To do so, I gave
the `State_space` class a `Reconfiguration_agent` field.
```diff
    bool use_supernodes = true;
+	Reconfiguration::Agent<Time> *reconfiguration_agent;

    State_space(const Workload& jobs,
        const Precedence_constraints& edges,
        const Abort_actions& aborts,
        unsigned int num_cpus,
        double max_cpu_time = 0,
        unsigned int max_depth = 0,
        bool early_exit = true,
 		bool use_supernodes = true,
+		Reconfiguration::Agent<Time> *reconfiguration_agent = nullptr)
        : jobs(jobs)
```
During normal explorations, the `reconfiguration_agent` will be `nullptr`, which means that the exploration is not
manipulated, and no extra data is stored.

### Manipulation
The reconfiguration agent is allowed to manipulate the exploration process by forbidding some edges to be taken
```diff
    Time t_high_wos = next_certain_higher_priority_seq_source_job_release(n, j, upbnd_t_wc + 1);
    // if there is a higher priority job that is certainly ready before job j is released at the earliest, 
    // then j will never be the next job dispached by the scheduler
    if (t_high_wos <= j.earliest_arrival())
        continue;
+
+   // The reconfiguration agent can forbid transitions
+   if (reconfiguration_agent && !reconfiguration_agent->is_allowed(n, j)) continue;
+
    found_one |= dispatch(n, j, upbnd_t_wc, t_high_wos);
```
or by forbidding the merge of two nodes.
```diff
for (Node_ref other : pair_it->second)
    {
        if (other->get_scheduled_jobs() != sched_jobs)
            continue;

+		if (reconfiguration_agent && !reconfiguration_agent->allow_merge(n, *other)) continue;
+
        // If we have reached here, it means that we have found an existing node with the same 
        // set of scheduled jobs than the new state resuting from scheduling job j in system state s.
        // Thus, our new state can be added to that existing node.
        if (other->merge_states(st, false))
        {
            delete& st;
            return *other;
        }
    }
```
Forbidding certain edges can be used to limit the exploration to the paths in which the agent is
interested. Forbidding merges can be used to e.g. forbid merges when exactly 1 of the two nodes has
already missed a deadline, which messes up the administration of some strategies.

### Data storage
The reconfiguration agent is also allowed to store arbitrary per-node data. I added an `attachment` to
the `Schedule_node` class:
```diff

public:
+   Reconfiguration::Attachment *attachment;

    // initial node
    Schedule_node(
        unsigned int num_cores,
        const Time next_earliest_release = 0,
        const Time next_certain_source_job_release = Time_model::constants<Time>::infinity(), // the next time a job without predecessor is certainly released
        const Time next_certain_sequential_source_job_release = Time_model::constants<Time>::infinity(), // the next time a job without predecessor that can execute on a single core is certainly released
+       Reconfiguration::Attachment *attachment = nullptr
    )
```
The `Reconfiguration::Attachment` struct is just an empty abstract struct, meant to be extended by subclasses.
The `State_space` allows the reconfiguration agent to create the attachments:
```diff
    Time next_certain_release = std::min(next_certain_seq_release, next_certain_gang_release);

+   Reconfiguration::Attachment *attachment = nullptr;
+   if (reconfiguration_agent) attachment = reconfiguration_agent->create_initial_node_attachment();

-   Node& n = new_node(num_cores, jobs_by_earliest_arrival.begin()->first, next_certain_release, next_certain_seq_release);
+   Node& n = new_node(num_cores, jobs_by_earliest_arrival.begin()->first, next_certain_release, next_certain_seq_release, attachment);
    State& s = new_state(num_cores, next_certain_gang_release);
```
```diff
+   Reconfiguration::Attachment *attachment = nullptr;
+   if (reconfiguration_agent) attachment = reconfiguration_agent->create_next_node_attachment(n, j);
+
    Node& next_node = new_node(n, j, j.get_job_index(),
        earliest_possible_job_release(n, j),
        earliest_certain_source_job_release(n, j),
        earliest_certain_sequential_source_job_release(n, j),
+       attachment);
```
The agents mostly use this to record the edges that lead to a node, which can be very useful (e.g. which sequence of jobs
lead to a deadline miss?).

### Signalling
Finally, the `State_space` will also signal deadline misses, dead ends, and completions to the reconfiguration agent.
```diff
if (j.exceeds_deadline(range.upto())) {
    observed_deadline_miss = true;
+   if (reconfiguration_agent) reconfiguration_agent->missed_deadline(n, j);

    if (early_exit)
        aborted = true;
}
```
```diff
    // check for a dead end
    if (!found_one && !all_jobs_scheduled(n)) {
        // out of options and we didn't schedule all jobs
        observed_deadline_miss = true;
-       aborted = true;
+       if (early_exit) aborted = true;
+       if (reconfiguration_agent) reconfiguration_agent->encountered_dead_end(n);
+   } else if (reconfiguration_agent && found_one && current_job_count == jobs.size() - 1) {
+       reconfiguration_agent->finished_node(n);
    }
```
This information is very useful, especially while also tracking the job sequence via the attachments:
it allows the agent to figure out which edges lead to deadline misses.

## Finding the 'bad' edges
Since every strategy so far needs to know which edges need to be avoided, the flow always starts with
finding all the edges that cause deadline misses. The `Agent_failure_search` class is used to find all
edges that lead to deadline misses or dead ends.

### Tracking the initial failures
It will store the sequence of scheduled jobs in the attachment of each node, and it maintains a list of 
*failures*, where each failure is a sequence of jobs that will lead to a deadline miss or dead end.
Whenever the `State_space` signals a dead end or deadline miss, the corresponding job sequence is added to
the list of failures.

### Finding the root failures
When all outgoing edges in a node lead to a failure, the node is basically a dead end,
and the edge leading to that node is also considered to be a failure. The job sequence leading to that
node will thus be added to the list of failures. Furthermore, all child sequences will be removed from
the list of failures, since they can't be reached when the parent job sequence is not taken.

The graph will be explored multiple times, until the list of failures converges.
The agent will also forbid the exploration from taking edges that can only lead to failures,
which reduces the search space in later iterations.

## The strategies
Once the program knows which edges should be avoided, the actual strategies will try to find a way
to avoid all these edges.

### The pessimism strategy
Since the scheduler is assumed to be non-deterministic, it is possible that failures occur only when
jobs do **not** run for their WCET. In such cases, the task set can become schedulable by making all
jobs (or a subset of jobs) always run for their WCET (the engineer can e.g. use busy waiting to
accomplish this). The pessimism strategy will try to make the problem schedulable by making some
jobs always use their WCET.

It starts by creating a copy of the problem, called the *adapted problem*, and changing the BCET of
all jobs to their WCET, and also change their best-case arrival times to their worst-case arrival
times. It will then call `State_space<Time>::explore` to check whether the *adapted problem* is
schedulable. If not, it will give up (and the program will try the next strategy).

If the *adapted problem* is schedulable, the strategy knows that the problem can be made schedulable
by simply assuming pessimistic execution times and arrival times. I could simply tell the user to
force all jobs to use their WCET and worst arrival time, but this is usually more work than needed.
In most cases, making only a few jobs pessimistic will fix the problem.

Ideally, I would find the minimum number of jobs that need to become pessimistic, but finding this is
probably an NP-hard problem. Instead, I will look for a local minimal number of jobs that need to
become pessimistic. It will use the following procedure:
```
for each job in the adapted problem:
  reset its BCET to the original BCET
  if the adapted problem is still schedulable:
    keep the original BCET
  else:
    change the BCET back to the WCET
    add the job to the list of pessimistic BCET jobs
  reset its BCAT to the original BCAT
  if the adapted problem is still schedulable:
    keep the original BCAT
  else:
    change the BCAT back to the WCAT
    add the job to the list of pessimistic BCAT jobs
```
Finally, it will print the list of BCET jobs and BCAT jobs to stdout. This will be a local minimal number
of jobs since removing the pessimism of any job will make the problem unschedulable. Since this strategy
managed to find a solution, no other strategies will be tried, and the problem quits.

### The precedence strategy
It is often possible to avoid bad edges by adding 'artificial' preceding constraints. Consider the case when
the edge from node *v* to *x* leads to a deadline miss. There are also edges from *v* to *y* and *v* to *z*
that don't lead to deadline misses. In this example, there are 2 solutions to fix the problem:
- Add an artificial precedence constraint to ensure that job *y* completes before job *x*
- Add an artificial precedence constraint to ensure that job *z* completes before job *x*

Either solution may or may not cause dead ends elsewhere in the graph. Currently, the strategy simply creates
an *adapted problem* with each precedence constraint that might work, and calls `State_space<Time>::explore`
to check whether it doesn't cause additional problems. As soon as it finds a constraint that works, it will
print the constraint to stdout, and quit the program. An example solution is given below. If all candidate
constraints fail, it will give up (and the program will attempt the next strategy).
```
The given problem is not schedulable, but you can make it schedulable by following these steps:
Add a precedence constraint to ensure that T1J2 must finish before T3J9 can start
```

**Note**: the current implementation of this strategy seems to work when there is exactly 1 failing edge,
but the implementation contains some potential flaws when there are multiple. I can definitely fix this,
but I should work on literature review first.

## Performance and memory
At this point, the code contains plenty of memory leaks and needless vector copies. Furthermore, it uses only
vectors rather than clever data structures. Thus, I suspect that it will take ridiculously much time when
tested on large problems.