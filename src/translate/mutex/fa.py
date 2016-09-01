import cplex

import common
import mutex

def fa(task, atoms, actions, rfa = False, rfa_bind_conflict = None,
       init_mutexes = None):
    atoms = common.filter_atoms(atoms)
    atoms_dict, atoms_list = common.create_atoms_dict(atoms)

    ilp = cplex.Cplex()
    ilp.objective.set_sense(ilp.objective.sense.maximize)
    ilp.set_log_stream(None)
    ilp.set_error_stream(None)
    ilp.set_results_stream(None)
    ilp.set_warning_stream(None)
    ilp.parameters.tune_problem([(ilp.parameters.threads, 1)])
    ilp.variables.add(obj = [1. for x in atoms_list],
                      names = [str(x) for x in atoms_list],
                      types = ['B' for x in atoms_list])

    # Initial state constraint
    s_init = list(set(task.init) & atoms)
    constr = cplex.SparsePair(ind = [atoms_dict[x] for x in s_init],
                              val = [1. for _ in s_init])
    ilp.linear_constraints.add(lin_expr = [constr], senses = ['L'], rhs = [1.])

    # Action constraints
    for action in actions:
        pre = set(action.precondition) & atoms
        add_eff = set([x[1] for x in action.add_effects]) & atoms
        del_eff = set([x[1] for x in action.del_effects]) & atoms
        pre = [atoms_dict[x] for x in pre if not x.negated]
        add_eff = [atoms_dict[x] for x in add_eff]
        del_eff = [atoms_dict[x] for x in del_eff]
        if len(add_eff) == 0:
            continue

        pre_del = list(set(pre) & set(del_eff))
        constr = cplex.SparsePair(ind = add_eff + pre_del,
                                  val = [1. for _ in add_eff]
                                        + [-1. for _ in pre_del])
        ilp.linear_constraints.add(lin_expr = [constr],
                                   senses = ['L'], rhs = [0.])
        if rfa:
            constr2 = cplex.SparsePair(ind = pre_del, val = [1. for _ in pre_del])
            ilp.linear_constraints.add(lin_expr = [constr2],
                                       senses = ['L'], rhs = [1.])

    # Bind and conflict set constraints
    if rfa_bind_conflict is not None:
        for f in rfa_bind_conflict:
            if f in f.conflict:
                constr = cplex.SparsePair(ind = [atoms_dict[f.fact]], val = [1.])
                ilp.linear_constraints.add(lin_expr = [constr],
                                           senses = ['E'], rhs = [0.])
                continue

            if len(f.bind) > 1:
                bind = [atoms_dict[x.fact] for x in f.bind - set([f])]
                cbind = cplex.SparsePair(ind = bind + [atoms_dict[f.fact]],
                                         val = [-1. for _ in bind] + [len(bind)])
                ilp.linear_constraints.add(lin_expr = [cbind],
                                           senses = ['L'], rhs = [0.])

            confl  = [atoms_dict[x.fact] for x in f.conflict]
            cconfl = cplex.SparsePair(ind = confl + [atoms_dict[f.fact]],
                                      val = [1. for _ in confl] + [len(confl)])
            ilp.linear_constraints.add(lin_expr = [cconfl],
                                       senses = ['L'], rhs = [len(confl)])


    if init_mutexes is not None:
        mx = sorted([m & atoms for m in init_mutexes], key = lambda x: -len(x))
        init_mutexes = []
        for m in mx:
            use = True
            for m2 in init_mutexes:
                if m.issubset(m2):
                    use = False
                    break
            if use:
                init_mutexes += [m]

        all_idx = set(range(len(atoms_list)))
        for m in init_mutexes:
            constr_idx = list(all_idx - set([atoms_dict[x] for x in m]))
            constr = cplex.SparsePair(ind = constr_idx,
                                      val = [1. for _ in constr_idx])
            ilp.linear_constraints.add(lin_expr = [constr],
                                       senses = ['G'], rhs = [1.])

    mutexes = set()
    ilp.solve()
    #print(c.solution.get_status())
    while ilp.solution.get_status() == 101:
        values = ilp.solution.get_values()
        if sum(values) <= 1.5:
            break
        sol = [atoms_list[x] for x in range(len(values)) if values[x] > 0.5]
        mutexes.add(frozenset(sol))

        # Add constraint removing all subsets of the current solution
        constr_idx = [x for x in range(len(values)) if values[x] < 0.5]
        constr = cplex.SparsePair(ind = constr_idx,
                                  val = [1. for _ in constr_idx])
        ilp.linear_constraints.add(lin_expr = [constr],
                                   senses = ['G'], rhs = [1.])
        ilp.solve()

    # filter out subsets
    if init_mutexes is not None:
        for m in init_mutexes:
            use = True
            for m2 in mutexes:
                if m.issubset(m2):
                    use = False
                    break
            if use:
                mutexes.add(frozenset(m))

    return mutexes, set()

def cfa(task, atoms, actions, rfa = False, rfa_bind_conflict = None):
    minit, matoms, mact, atoms = common.preprocess_input(task, atoms, actions)
    g = mutex.fa(minit, matoms, mact)
    g = [[atoms[x] for x in y] for y in g]
    g = set([frozenset(x) for x in g])
    return g, set()
