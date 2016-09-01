from __future__ import print_function
import sys
import resource

import common

class FullAction(object):
    def __init__(self, a, atoms):
        self.name = a.name
        self.pre = set(a.precondition) & atoms
        add_eff = [x[1] for x in a.add_effects if len(x[0]) == 0]
        del_eff = [x[1] for x in a.del_effects if len(x[0]) == 0]
        self.add_eff = set(add_eff) & atoms
        self.del_eff = set(del_eff) & atoms
        # TODO: Conditional effects

def action_apply(a, state, states_out):
    if a.pre.issubset(state):
        news = (state - a.del_eff) | a.add_eff
        states_out.add(news)

def full(task, atoms, actions, verbose = False, max_states = -1,
         max_time = -1., max_mem = -1.):
    atoms = common.filter_atoms(atoms)
    init = set(task.init) & atoms
    actions = [FullAction(a, atoms) for a in actions]

    unreached_facts = set(atoms)
    unreached_pairs = common.gen_all_pairs([atoms])
    reached_states = set()

    states = set([frozenset(init)])
    count = 0
    while len(states) > 0:
        #print(reached_states)
        next_states = set()
        for s in states:
            for a in actions:
                action_apply(a, s, next_states)
                if count == 1000 and (max_time > 0. or max_mem > 0.):
                    res = resource.getrusage(resource.RUSAGE_SELF)
                    tim = res.ru_utime + res.ru_stime
                    mem = res.ru_maxrss / 1024.
                    if max_time > 0. and tim > max_time:
                        print('mutex.full(): Reached time limit ({0} s). Terminating...'.format(tim))
                        sys.exit(-1)

                    if max_mem > 0. and mem > max_mem:
                        print('mutex.full(): Reached mem limit ({0} MB). Terminating...'.format(mem))
                        sys.exit(-1)

                    count = 0
                count += 1
            unreached_facts -= s
            unreached_pairs -= common.gen_all_pairs([s])
            reached_states.add(s)
        states = next_states - reached_states

        if verbose:
            res = resource.getrusage(resource.RUSAGE_SELF)
            tim = res.ru_utime + res.ru_stime
            mem = res.ru_maxrss / 1024.
            print('mutex.full():: Reached states: {0}, mem: {1:.2f} MB, time: {2} s' \
                    .format(len(reached_states), mem, tim))

        if max_states >= 0 and len(reached_states) > max_states:
            print('mutex.full():: Exceeded max-states ({0}/{1}).  Aborted.'
                    .format(len(reached_states), max_states))
            return None, None

    mutexes = unreached_pairs
    return mutexes, unreached_facts


def check_mutexes(all_mutexes, mutexes):
    mutexes = common.gen_all_pairs(mutexes)
    invalid = mutexes - all_mutexes
    if len(invalid) > 0:
        print('ERR: Invalid mutexes:')
        for m in invalid:
            print('\t', m)
        sys.exit(-1)
    return True
