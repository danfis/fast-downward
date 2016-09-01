import cplex
from itertools import combinations as comb

import common
import fa

class RFAAction(object):
    def __init__(self, action, atoms, atom_to_fact):
        self.name = action.name
        pre = set(action.precondition) & atoms
        add_eff = set([x[1] for x in action.add_effects]) & atoms
        del_eff = set([x[1] for x in action.del_effects]) & atoms
        self.pre = set([atom_to_fact[x] for x in pre if not x.negated])
        self.add_eff = set([atom_to_fact[x] for x in add_eff])
        self.del_eff = set([atom_to_fact[x] for x in del_eff])
        self.pre_del = self.pre & self.del_eff

class RFAFact(object):
    def __init__(self, fact, atoms):
        self.fact = fact
        self.all_facts = None
        self.actions = None

        self.bind = set([self])
        self.conflict = set()
        self.useless = False

    def set_all_facts(self, facts):
        self.all_facts = facts
    def set_actions(self, actions):
        self.actions = actions

    def add_conflict(self, f2):
        if f2 not in self.conflict:
            self.conflict.add(f2)
            f2.conflict.add(self)
            return True
        return False

    def add_bind(self, f2):
        if f2.useless:
            self.set_useless()
            return True

        elif f2 not in self.bind:
            self.bind.add(f2)
            return True
        return False

    def set_useless(self):
        if not self.useless:
            self.useless = True
            for f in self.all_facts:
                self.add_bind(f)
                self.add_conflict(f)
            return True

        return False

    def apply_transitivity(self):
        if self.useless:
            return False

        change = False
        for f2 in set(self.bind):
            for f3 in f2.bind:
                change |= self.add_bind(f3)
            for f3 in set(f2.conflict):
                change |= self.add_conflict(f3)
        return change

    def check_useless(self):
        if not self.useless and len(self.bind & self.conflict) > 0:
            return self.set_useless()
        return False

    def check_actions(self):
        if self.useless:
            return False

        change = False
        pre_del = set()
        for a in self.actions:
            if len(a.add_eff & self.bind) > 0:
                pd = a.pre_del - self.conflict
                if len(pd) == 0:
                    return self.set_useless()
                if len(pd) == 1:
                    change |= self.add_bind(list(pd)[0])
                pre_del.add(frozenset(pd))

        for pd1, pd2 in comb(pre_del, 2):
            if pd1.issubset(pd2):
                for f in pd2 - pd1:
                    change |= self.add_conflict(f)
            if pd2.issubset(pd1):
                for f in pd1 - pd2:
                    change |= self.add_conflict(f)

        return change

    def check_rfa(self):
        for a in self.actions:
            add_size = len(a.add_eff & self.bind)
            pre_del_size = len(a.pre_del & self.bind)
            if add_size > 1 or pre_del_size > 1 or pre_del_size > add_size:
                return False
        return True


def rfa_init(atom_to_fact, task, atoms, actions):
    s_init = [atom_to_fact[x] for x in set(task.init) & set(atoms)]
    for f1, f2 in comb(s_init, 2):
        f1.add_conflict(f2)

    for a in actions:
        for f1, f2 in comb(a.add_eff, 2):
            f1.add_conflict(f2)
        for f1, f2 in comb(a.pre_del, 2):
            f1.add_conflict(f2)


def rfa_conflict_bind(task, atoms, actions):
    atoms = common.filter_atoms(atoms)
    atom_to_fact = { a : RFAFact(a, atoms) for a in atoms }
    facts = atom_to_fact.values()
    actions = [RFAAction(a, atoms, atom_to_fact) for a in actions]
    [(f.set_all_facts(facts), f.set_actions(actions)) for f in facts]

    rfa_init(atom_to_fact, task, atoms, actions)

    change = True
    while change:
        change = False
        for f in facts:
            change |= f.apply_transitivity()
            change |= f.check_useless()
            change |= f.check_actions()
    return facts

def rfa(task, atoms, actions):
    facts = rfa_conflict_bind(task, atoms, actions)
    rfa = set()
    for f in facts:
        if f.check_rfa():
            rfa.add(frozenset([x.fact for x in f.bind]))
    return rfa, set()

def rfa_complete(task, atoms, actions):
    facts = rfa_conflict_bind(task, atoms, actions)
    return fa.fa(task, atoms, actions, True, facts)
