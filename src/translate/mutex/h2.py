import sys
import itertools
from pprint import pprint

import common
import mutex

class H2Action(object):
    def __init__(self, a, atoms):
        self.name = a.name
        self.applied = False

        pre = set(a.precondition) & atoms
        add_eff = set([x[1] for x in a.add_effects if len(x[0]) == 0]) & atoms
        del_eff = set([x[1] for x in a.del_effects if len(x[0]) == 0]) & atoms
        self.pre1 = set(pre)
        self.add_eff1 = set(add_eff)
        self.add_del = set(add_eff) | set(del_eff)

        self.pre2 = common.gen_all_pairs([self.pre1])
        p = self.pre1 - self.add_del
        self.add_eff2 = common.gen_all_pairs([self.add_eff1 | p])

        self.pre = self.pre1 | self.pre2
        self.add_eff = self.add_eff1 | self.add_eff2

    def pairs(self, fact, facts):
        for f in facts:
            yield frozenset([fact, f])

    def eff(self, reached1, reached2):
        if not self.applied:
            r1 = self.add_eff1
            r2 = self.add_eff2
        else:
            r1 = set()
            r2 = set()

        atoms = reached1 - self.add_del
        for atom in atoms:
            pre = set(self.pairs(atom, self.pre1))
            if pre.issubset(reached2):
                for atom2 in self.add_eff1:
                    t = frozenset([atom, atom2])
                    r2.add(t)
                self.add_del.add(atom)

        self.applied = True
        return r1, r2


def h2(task, atoms, actions):
    atoms = common.filter_atoms(atoms)
    actions = [H2Action(a, atoms) for a in actions]

    init = set(task.init) & atoms

    reached1 = init
    reached2 = common.gen_all_pairs([init])
    lastsize = 0
    while len(reached1) + len(reached2) != lastsize:
        lastsize = len(reached1) + len(reached2)

        for a in actions:
            if a.pre1.issubset(reached1) and a.pre2.issubset(reached2):
                r1, r2 = a.eff(reached1, reached2)
                reached1 |= r1
                reached2 |= r2

    m2 = common.gen_all_pairs([atoms]) - reached2
    return m2, atoms - reached1

def h2_max(task, atoms, actions):
    h2_mutex, unreachable = h2(task, atoms, actions)
    return common.max_mutexes_from_pair_mutexes(h2_mutex)

def ch2(task, atoms, actions):
    minit, matoms, mact, atoms = common.preprocess_input(task, atoms, actions)
    h2, h1 = mutex.h2(minit, matoms, mact)
    h2 = [[atoms[x] for x in y] for y in h2]
    h2 = set([frozenset(x) for x in h2])
    h1 = set([atoms[x] for x in h1])
    return h2, h1
