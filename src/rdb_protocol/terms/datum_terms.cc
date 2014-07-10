// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "rdb_protocol/op.hpp"

namespace ql {

class datum_term_t : public term_t {
public:
    explicit datum_term_t(protob_t<const Term> t)
        : term_t(t), raw_val(new_val(make_counted<const datum_t>(&t->datum()))) { }
private:
    void accumulate_captures(var_captures_t *) const FINAL { /* do nothing */ }
    bool is_deterministic() const FINAL { return true; }
    int parallelization_level() const FINAL { return 0; }
    counted_t<val_t> term_eval(scope_env_t *, eval_flags_t) const FINAL {
        return raw_val;
    }
    const char *name() const FINAL { return "datum"; }
    counted_t<val_t> raw_val;
};

// RSI: How is this an op_term_t at all?
class constant_term_t : public op_term_t {
public:
    constant_term_t(compile_env_t *env, protob_t<const Term> t,
                    double constant, const char *name)
        : op_term_t(env, t, argspec_t(0)), constant_(constant), name_(name) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *, args_t *, eval_flags_t) const FINAL {
        return new_val(make_counted<const datum_t>(constant_));
    }
    const char *name() const FINAL { return name_; }

    int parallelization_level() const FINAL {
        return params_parallelization_level();
    }

    bool op_is_deterministic() const FINAL { return true; }

    const double constant_;
    const char *const name_;
};

// RSI: How is this an op_term_t at all?
class make_array_term_t : public op_term_t {
public:
    make_array_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(0, -1)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const FINAL {
        datum_ptr_t acc(datum_t::R_ARRAY);
        {
            profile::sampler_t sampler("Evaluating elements in make_array.", env->env->trace);
            for (size_t i = 0; i < args->num_args(); ++i) {
                acc.add(args->arg(env, i)->as_datum());
                sampler.new_sample();
            }
        }
        return new_val(acc.to_counted());
    }
    const char *name() const FINAL { return "make_array"; }

    bool op_is_deterministic() const FINAL { return true; }

    int parallelization_level() const FINAL {
        return params_parallelization_level();
    }
};

class make_obj_term_t : public term_t {
public:
    make_obj_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : term_t(term) {
        // An F.Y.I. for driver developers.
        rcheck(term->args_size() == 0,
               base_exc_t::GENERIC,
               "MAKE_OBJ term must not have any args.");

        for (int i = 0; i < term->optargs_size(); ++i) {
            const Term_AssocPair *pair = &term->optargs(i);
            counted_t<const term_t> t = compile_term(env, term.make_child(&pair->val()));
            auto res = optargs.insert(std::make_pair(pair->key(), std::move(t)));
            rcheck(res.second,
                   base_exc_t::GENERIC,
                   strprintf("Duplicate object key: %s",
                             pair->key().c_str()));
        }
    }

    counted_t<val_t> term_eval(scope_env_t *env, eval_flags_t flags) const {
        bool literal_ok = flags & LITERAL_OK;
        eval_flags_t new_flags = literal_ok ? LITERAL_OK : NO_FLAGS;
        datum_ptr_t acc(datum_t::R_OBJECT);
        {
            profile::sampler_t sampler("Evaluating elements in make_obj.", env->env->trace);
            for (auto it = optargs.begin(); it != optargs.end(); ++it) {
                bool dup = acc.add(it->first, it->second->eval(env, new_flags)->as_datum());
                rcheck(!dup, base_exc_t::GENERIC,
                       strprintf("Duplicate object key: %s.", it->first.c_str()));
                sampler.new_sample();
            }
        }
        return new_val(acc.to_counted());
    }

    int parallelization_level() const FINAL {
        return max_parallelization_level(optargs);
    }

    bool is_deterministic() const FINAL {
        return all_are_deterministic(optargs);
    }

    void accumulate_captures(var_captures_t *captures) const {
        accumulate_all_captures(optargs, captures);
    }

    const char *name() const { return "make_obj"; }

private:
    std::map<std::string, counted_t<const term_t> > optargs;
    DISABLE_COPYING(make_obj_term_t);
};

counted_t<term_t> make_datum_term(const protob_t<const Term> &term) {
    return make_counted<datum_term_t>(term);
}
counted_t<term_t> make_constant_term(compile_env_t *env, const protob_t<const Term> &term,
                                     double constant, const char *name) {
    return make_counted<constant_term_t>(env, term, constant, name);
}
counted_t<term_t> make_make_array_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<make_array_term_t>(env, term);
}
counted_t<term_t> make_make_obj_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<make_obj_term_t>(env, term);
}

} // namespace ql
