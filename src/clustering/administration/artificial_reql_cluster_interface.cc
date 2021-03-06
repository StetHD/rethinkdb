// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/artificial_reql_cluster_interface.hpp"

#include "clustering/administration/auth/grant.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/real_reql_cluster_interface.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "rdb_protocol/artificial_table/artificial_table.hpp"
#include "rdb_protocol/env.hpp"
#include "rpc/semilattice/view/field.hpp"

/* static */ const name_string_t artificial_reql_cluster_interface_t::database_name =
    name_string_t::guarantee_valid("rethinkdb");

// Note, can't use the above as order of initialization isn't guaranteed
/* static */ const uuid_u artificial_reql_cluster_interface_t::database_id =
    uuid_u::from_hash(str_to_uuid("39a24924-14ec-4deb-99f1-742eda7aba5e"), "rethinkdb");

artificial_reql_cluster_interface_t::artificial_reql_cluster_interface_t(
        boost::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t>>
            auth_semilattice_view,
        rdb_context_t *rdb_context):
    m_auth_semilattice_view(auth_semilattice_view),
    m_rdb_context(rdb_context),
    m_next(nullptr) {
}

bool artificial_reql_cluster_interface_t::db_create(
        auth::user_context_t const &user_context,
        const name_string_t &name,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` already exists.", artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->db_create(
        user_context, name, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_drop(
        auth::user_context_t const &user_context,
        const name_string_t &name,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't delete it.",
                      artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->db_drop(
        user_context, name, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_list(
        signal_t *interruptor,
        std::set<name_string_t> *names_out, admin_err_t *error_out) {
    if (m_next == nullptr || !m_next->db_list(interruptor, names_out, error_out)) {
        return false;
    }
    guarantee(names_out->count(artificial_reql_cluster_interface_t::database_name) == 0);
    names_out->insert(artificial_reql_cluster_interface_t::database_name);
    return true;
}

bool artificial_reql_cluster_interface_t::db_find(
        const name_string_t &name,
        signal_t *interruptor,
        counted_t<const ql::db_t> *db_out,
        admin_err_t *error_out) {
    if (name == artificial_reql_cluster_interface_t::database_name) {
        *db_out = make_counted<const ql::db_t>(artificial_reql_cluster_interface_t::database_id, artificial_reql_cluster_interface_t::database_name);
        return true;
    }
    return next_or_error(error_out) && m_next->db_find(name, interruptor, db_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_config(
        auth::user_context_t const &user_context,
        const counted_t<const ql::db_t> &db,
        ql::backtrace_id_t bt,
        ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out,
        admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't configure it.",
                      artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->db_config(
        user_context, db, bt, env, selection_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_create(
        auth::user_context_t const &user_context,
        const name_string_t &name,
        counted_t<const ql::db_t> db,
        const table_generate_config_params_t &config_params,
        const std::string &primary_key,
        write_durability_t durability,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't create new tables "
                      "in it.", artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->table_create(
        user_context,
        name,
        db,
        config_params,
        primary_key,
        durability,
        interruptor,
        result_out,
        error_out);
}

bool artificial_reql_cluster_interface_t::table_drop(
        auth::user_context_t const &user_context,
        const name_string_t &name,
        counted_t<const ql::db_t> db,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't drop tables in it.",
                      artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->table_drop(
        user_context, name, db, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_list(counted_t<const ql::db_t> db,
        signal_t *interruptor,
        std::set<name_string_t> *names_out, admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        for (auto it = m_table_backends.begin(); it != m_table_backends.end(); ++it) {
            if (it->first.str()[0] == '_') {
                /* If a table's name starts with `_`, don't show it to the user unless
                they explicitly request it. */
                continue;
            }
            names_out->insert(it->first);
        }
        return true;
    }
    return next_or_error(error_out) && m_next->table_list(
        db, interruptor, names_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_find(
        const name_string_t &name,
        counted_t<const ql::db_t> db,
        boost::optional<admin_identifier_format_t> identifier_format,
        signal_t *interruptor,
        counted_t<base_table_t> *table_out,
        admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        auto it = m_table_backends.find(name);
        if (it != m_table_backends.end()) {
            artificial_table_backend_t *backend;
            if (!static_cast<bool>(identifier_format) ||
                    *identifier_format == admin_identifier_format_t::name) {
                backend = it->second.first;
            } else {
                backend = it->second.second;
            }
            table_out->reset(
                new artificial_table_t(m_rdb_context, artificial_reql_cluster_interface_t::database_id, backend));
            return true;
        } else {
            *error_out = admin_err_t{
                strprintf("Table `%s.%s` does not exist.",
                          artificial_reql_cluster_interface_t::database_name.c_str(), name.c_str()),
                query_state_t::FAILED};
            return false;
        }
    }
    return next_or_error(error_out) && m_next->table_find(
        name, db, identifier_format, interruptor, table_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_estimate_doc_counts(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        ql::env_t *env,
        std::vector<int64_t> *doc_counts_out,
        admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        auto it = m_table_backends.find(name);
        if (it != m_table_backends.end()) {
            counted_t<ql::datum_stream_t> docs;
            /* We arbitrarily choose to read from the UUID version of the system table
            rather than the name version. */
            if (!it->second.second->read_all_rows_as_stream(
                    user_context,
                    ql::backtrace_id_t::empty(),
                    ql::datumspec_t(ql::datum_range_t::universe()),
                    sorting_t::UNORDERED,
                    env->interruptor,
                    &docs,
                    error_out)) {
                error_out->msg = "When estimating doc count: " + error_out->msg;
                return false;
            }
            try {
                scoped_ptr_t<ql::val_t> count =
                    docs->run_terminal(env, ql::count_wire_func_t());
                *doc_counts_out = std::vector<int64_t>({ count->as_int<int64_t>() });
            } catch (const ql::base_exc_t &msg) {
                *error_out = admin_err_t{
                    "When estimating doc count: " + std::string(msg.what()),
                    query_state_t::FAILED};
                return false;
            }
            return true;
        } else {
            *error_out = admin_err_t{
                strprintf("Table `%s.%s` does not exist.",
                          artificial_reql_cluster_interface_t::database_name.c_str(), name.c_str()),
                query_state_t::FAILED};
            return false;
        }
    } else {
        return next_or_error(error_out) && m_next->table_estimate_doc_counts(
            user_context, db, name, env, doc_counts_out, error_out);
    }
}

bool artificial_reql_cluster_interface_t::table_config(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        ql::backtrace_id_t bt,
        ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out, admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't configure the "
                      "tables in it.", artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->table_config(
        user_context, db, name, bt, env, selection_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_status(
        counted_t<const ql::db_t> db, const name_string_t &name,
        ql::backtrace_id_t bt, ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out, admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; the system tables in it don't "
                      "have meaningful status information.", artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->table_status(
        db, name, bt, env, selection_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_wait(
        counted_t<const ql::db_t> db, const name_string_t &name,
        table_readiness_t readiness, signal_t *interruptor,
        ql::datum_t *result_out, admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; the system tables in it are "
                      "always available and don't need to be waited on.",
                      artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->table_wait(
        db, name, readiness, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_wait(
        counted_t<const ql::db_t> db, table_readiness_t readiness,
        signal_t *interruptor,
        ql::datum_t *result_out, admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; the system tables in it are "
                      "always available and don't need to be waited on.",
                      artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->db_wait(
        db, readiness, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_reconfigure(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        const table_generate_config_params_t &params,
        bool dry_run,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't configure the "
                      "tables in it.", artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->table_reconfigure(
        user_context, db, name, params, dry_run, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_reconfigure(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const table_generate_config_params_t &params,
        bool dry_run,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't configure the "
                      "tables in it.", artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->db_reconfigure(
        user_context, db, params, dry_run, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_emergency_repair(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        emergency_repair_mode_t mode,
        bool dry_run,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't configure the "
                      "tables in it.", artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->table_emergency_repair(
        user_context, db, name, mode, dry_run, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_rebalance(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't rebalance the "
                      "tables in it.", artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->table_rebalance(
        user_context, db, name, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_rebalance(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't rebalance the "
                      "tables in it.", artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->db_rebalance(
        user_context, db, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::grant_global(
        auth::user_context_t const &user_context,
        auth::username_t username,
        ql::datum_t permissions,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    return next_or_error(error_out) && m_next->grant_global(
        user_context,
        std::move(username),
        std::move(permissions),
        interruptor,
        result_out,
        error_out);
}

bool artificial_reql_cluster_interface_t::grant_database(
        auth::user_context_t const &user_context,
        database_id_t const &database,
        auth::username_t username,
        ql::datum_t permissions,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (database == artificial_reql_cluster_interface_t::database_id) {
        cross_thread_signal_t cross_thread_interruptor(interruptor, home_thread());
        on_thread_t on_thread(home_thread());

        return auth::grant(
            m_auth_semilattice_view,
            m_rdb_context,
            user_context,
            std::move(username),
            std::move(permissions),
            &cross_thread_interruptor,
            [&](auth::user_t &user) -> auth::permissions_t & {
                return user.get_database_permissions(database);
            },
            result_out,
            error_out);
    }
    return next_or_error(error_out) && m_next->grant_database(
        user_context,
        database,
        std::move(username),
        std::move(permissions),
        interruptor,
        result_out,
        error_out);
}

bool artificial_reql_cluster_interface_t::grant_table(
        auth::user_context_t const &user_context,
        database_id_t const &database,
        namespace_id_t const &table,
        auth::username_t username,
        ql::datum_t permissions,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (database == artificial_reql_cluster_interface_t::database_id) {
        cross_thread_signal_t cross_thread_interruptor(interruptor, home_thread());
        on_thread_t on_thread(home_thread());

        return auth::grant(
            m_auth_semilattice_view,
            m_rdb_context,
            user_context,
            std::move(username),
            std::move(permissions),
            &cross_thread_interruptor,
            [&](auth::user_t &user) -> auth::permissions_t & {
                return user.get_table_permissions(table);
            },
            result_out,
            error_out);
    }
    return next_or_error(error_out) && m_next->grant_table(
        user_context,
        database,
        table,
        std::move(username),
        std::move(permissions),
        interruptor,
        result_out,
        error_out);
}

bool artificial_reql_cluster_interface_t::set_write_hook(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        boost::optional<write_hook_config_t> &config,
        signal_t *interruptor,
        admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't set a "
                      "write hook on the tables in it.",
                      artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->set_write_hook(
        user_context, db, table, config, interruptor, error_out);
}

bool artificial_reql_cluster_interface_t::get_write_hook(
    auth::user_context_t const &user_context,
    counted_t<const ql::db_t> db,
    const name_string_t &table,
    signal_t *interruptor,
    ql::datum_t *write_hook_datum_out,
    admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *write_hook_datum_out = ql::datum_t::null();
        return true;
    }
    return m_next->get_write_hook(
        user_context, db, table, interruptor, write_hook_datum_out, error_out);
}
bool artificial_reql_cluster_interface_t::sindex_create(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        const std::string &name,
        const sindex_config_t &config,
        signal_t *interruptor,
        admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't create secondary "
                      "indexes on the tables in it.", artificial_reql_cluster_interface_t::database_name.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->sindex_create(
        user_context, db, table, name, config, interruptor, error_out);
}

bool artificial_reql_cluster_interface_t::sindex_drop(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        const std::string &name,
        signal_t *interruptor,
        admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Index `%s` does not exist on table `%s.%s`.",
                      name.c_str(), db->name.c_str(), table.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->sindex_drop(
        user_context, db, table, name, interruptor, error_out);
}

bool artificial_reql_cluster_interface_t::sindex_rename(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        const std::string &name,
        const std::string &new_name,
        bool overwrite,
        signal_t *interruptor,
        admin_err_t *error_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        *error_out = admin_err_t{
            strprintf("Index `%s` does not exist on table `%s.%s`.",
                      name.c_str(), db->name.c_str(), table.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return next_or_error(error_out) && m_next->sindex_rename(
        user_context, db, table, name, new_name, overwrite, interruptor, error_out);
}

bool artificial_reql_cluster_interface_t::sindex_list(
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        signal_t *interruptor,
        admin_err_t *error_out,
        std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
            *configs_and_statuses_out) {
    if (db->name == artificial_reql_cluster_interface_t::database_name) {
        configs_and_statuses_out->clear();
        return true;
    }
    return next_or_error(error_out) && m_next->sindex_list(
        db, table, interruptor, error_out, configs_and_statuses_out);
}

void artificial_reql_cluster_interface_t::set_next_reql_cluster_interface(
        reql_cluster_interface_t *next) {
    m_next = next;
}

artificial_table_backend_t *artificial_reql_cluster_interface_t::get_table_backend(
        name_string_t const &table_name,
        admin_identifier_format_t admin_identifier_format) const {
    auto table_backend = m_table_backends.find(table_name);
    if (table_backend != m_table_backends.end()) {
        switch (admin_identifier_format) {
            case admin_identifier_format_t::name:
                return table_backend->second.first;
                break;
            case admin_identifier_format_t::uuid:
                return table_backend->second.second;
                break;
        }
    } else {
        return nullptr;
    }
}

artificial_reql_cluster_interface_t::table_backends_map_t *
artificial_reql_cluster_interface_t::get_table_backends_map_mutable() {
    return &m_table_backends;
}

artificial_reql_cluster_interface_t::table_backends_map_t const &
artificial_reql_cluster_interface_t::get_table_backends_map() const {
    return m_table_backends;
}

bool artificial_reql_cluster_interface_t::next_or_error(admin_err_t *error_out) const {
    if (m_next == nullptr) {
        if (error_out != nullptr) {
            *error_out = admin_err_t{
                "Failed to find an interface.", query_state_t::FAILED};
        }
        return false;
    }
    return true;
}

artificial_reql_cluster_backends_t::artificial_reql_cluster_backends_t(
        artificial_reql_cluster_interface_t *artificial_reql_cluster_interface,
        real_reql_cluster_interface_t *real_reql_cluster_interface,
        boost::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t>>
            auth_semilattice_view,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t>>
            cluster_semilattice_view,
        boost::shared_ptr<semilattice_readwrite_view_t<heartbeat_semilattice_metadata_t>>
            heartbeat_semilattice_view,
        clone_ptr_t<watchable_t<change_tracking_map_t<
            peer_id_t, cluster_directory_metadata_t>>> directory_view,
        watchable_map_t<peer_id_t, cluster_directory_metadata_t> *directory_map_view,
        table_meta_client_t *table_meta_client,
        server_config_client_t *server_config_client,
        mailbox_manager_t *mailbox_manager,
        rdb_context_t *rdb_context,
        lifetime_t<name_resolver_t const &> name_resolver) {
    for (int format = 0; format < 2; ++format) {
        permissions_backend[format].init(
            new auth::permissions_artificial_table_backend_t(
                rdb_context,
                name_resolver,
                auth_semilattice_view,
                cluster_semilattice_view,
                static_cast<admin_identifier_format_t>(format)));
    }
    permissions_sentry = backend_sentry_t(
        artificial_reql_cluster_interface->get_table_backends_map_mutable(),
        name_string_t::guarantee_valid("permissions"),
        std::make_pair(permissions_backend[0].get(), permissions_backend[1].get()));

    users_backend.init(
        new auth::users_artificial_table_backend_t(
            rdb_context,
            name_resolver,
            auth_semilattice_view,
            cluster_semilattice_view));
    users_sentry = backend_sentry_t(
        artificial_reql_cluster_interface->get_table_backends_map_mutable(),
        name_string_t::guarantee_valid("users"),
        std::make_pair(users_backend.get(), users_backend.get()));

    cluster_config_backend.init(
        new cluster_config_artificial_table_backend_t(
            rdb_context,
            name_resolver,
            heartbeat_semilattice_view));
    cluster_config_sentry = backend_sentry_t(
        artificial_reql_cluster_interface->get_table_backends_map_mutable(),
        name_string_t::guarantee_valid("cluster_config"),
        std::make_pair(cluster_config_backend.get(), cluster_config_backend.get()));

    db_config_backend.init(
        new db_config_artificial_table_backend_t(
            rdb_context,
            name_resolver,
            metadata_field(
                &cluster_semilattice_metadata_t::databases,
                cluster_semilattice_view),
            real_reql_cluster_interface));
    db_config_sentry = backend_sentry_t(
        artificial_reql_cluster_interface->get_table_backends_map_mutable(),
        name_string_t::guarantee_valid("db_config"),
        std::make_pair(db_config_backend.get(), db_config_backend.get()));

    for (int format = 0; format < 2; ++format) {
        issues_backend[format].init(
            new issues_artificial_table_backend_t(
                rdb_context,
                name_resolver,
                mailbox_manager,
                cluster_semilattice_view,
                directory_map_view,
                server_config_client,
                table_meta_client,
                real_reql_cluster_interface->get_namespace_repo(),
                static_cast<admin_identifier_format_t>(format)));
    }
    issues_sentry = backend_sentry_t(
        artificial_reql_cluster_interface->get_table_backends_map_mutable(),
        name_string_t::guarantee_valid("current_issues"),
        std::make_pair(issues_backend[0].get(), issues_backend[1].get()));

    for (int format = 0; format < 2; ++format) {
        logs_backend[format].init(
            new logs_artificial_table_backend_t(
                rdb_context,
                name_resolver,
                mailbox_manager,
                directory_map_view,
                server_config_client,
                static_cast<admin_identifier_format_t>(format)));
    }
    logs_sentry = backend_sentry_t(
        artificial_reql_cluster_interface->get_table_backends_map_mutable(),
        name_string_t::guarantee_valid("logs"),
        std::make_pair(logs_backend[0].get(), logs_backend[1].get()));

    server_config_backend.init(
        new server_config_artificial_table_backend_t(
            rdb_context,
            name_resolver,
            directory_map_view,
            server_config_client));
    server_config_sentry = backend_sentry_t(
        artificial_reql_cluster_interface->get_table_backends_map_mutable(),
        name_string_t::guarantee_valid("server_config"),
        std::make_pair(server_config_backend.get(), server_config_backend.get()));

    for (int format = 0; format < 2; ++format) {
        server_status_backend[format].init(
            new server_status_artificial_table_backend_t(
                rdb_context,
                name_resolver,
                directory_map_view,
                server_config_client,
                static_cast<admin_identifier_format_t>(format)));
    }
    server_status_sentry = backend_sentry_t(
        artificial_reql_cluster_interface->get_table_backends_map_mutable(),
        name_string_t::guarantee_valid("server_status"),
        std::make_pair(server_status_backend[0].get(), server_status_backend[1].get()));

    for (int format = 0; format < 2; ++format) {
        stats_backend[format].init(
            new stats_artificial_table_backend_t(
                rdb_context,
                name_resolver,
                directory_view,
                cluster_semilattice_view,
                server_config_client,
                table_meta_client,
                mailbox_manager,
                static_cast<admin_identifier_format_t>(format)));
    }
    stats_sentry = backend_sentry_t(
        artificial_reql_cluster_interface->get_table_backends_map_mutable(),
        name_string_t::guarantee_valid("stats"),
        std::make_pair(stats_backend[0].get(), stats_backend[1].get()));

    for (int format = 0; format < 2; ++format) {
        table_config_backend[format].init(
            new table_config_artificial_table_backend_t(
                rdb_context,
                name_resolver,
                cluster_semilattice_view,
                real_reql_cluster_interface,
                static_cast<admin_identifier_format_t>(format),
                server_config_client,
                table_meta_client));
    }
    table_config_sentry = backend_sentry_t(
        artificial_reql_cluster_interface->get_table_backends_map_mutable(),
        name_string_t::guarantee_valid("table_config"),
        std::make_pair(table_config_backend[0].get(), table_config_backend[1].get()));

    for (int format = 0; format < 2; ++format) {
        table_status_backend[format].init(
            new table_status_artificial_table_backend_t(
                rdb_context,
                name_resolver,
                cluster_semilattice_view,
                server_config_client,
                table_meta_client,
                real_reql_cluster_interface->get_namespace_repo(),
                static_cast<admin_identifier_format_t>(format)));
    }
    table_status_sentry = backend_sentry_t(
        artificial_reql_cluster_interface->get_table_backends_map_mutable(),
        name_string_t::guarantee_valid("table_status"),
        std::make_pair(table_status_backend[0].get(), table_status_backend[1].get()));

    for (int format = 0; format < 2; ++format) {
        jobs_backend[format].init(
            new jobs_artificial_table_backend_t(
                rdb_context,
                name_resolver,
                mailbox_manager,
                cluster_semilattice_view,
                directory_view,
                server_config_client,
                table_meta_client,
                static_cast<admin_identifier_format_t>(format)));
    }
    jobs_sentry = backend_sentry_t(
        artificial_reql_cluster_interface->get_table_backends_map_mutable(),
        name_string_t::guarantee_valid("jobs"),
        std::make_pair(jobs_backend[0].get(), jobs_backend[1].get()));

    debug_scratch_backend.init(
        new in_memory_artificial_table_backend_t(
            name_string_t::guarantee_valid("_debug_scratch"),
            rdb_context,
            name_resolver));
    debug_scratch_sentry = backend_sentry_t(
        artificial_reql_cluster_interface->get_table_backends_map_mutable(),
        name_string_t::guarantee_valid("_debug_scratch"),
        std::make_pair(debug_scratch_backend.get(), debug_scratch_backend.get()));

    debug_stats_backend.init(
        new debug_stats_artificial_table_backend_t(
            rdb_context,
            name_resolver,
            directory_map_view,
            server_config_client,
            mailbox_manager));
    debug_stats_sentry = backend_sentry_t(
        artificial_reql_cluster_interface->get_table_backends_map_mutable(),
        name_string_t::guarantee_valid("_debug_stats"),
        std::make_pair(debug_stats_backend.get(), debug_stats_backend.get()));

    debug_table_status_backend.init(
        new debug_table_status_artificial_table_backend_t(
            rdb_context,
            name_resolver,
            cluster_semilattice_view,
            table_meta_client));
    debug_table_status_sentry = backend_sentry_t(
        artificial_reql_cluster_interface->get_table_backends_map_mutable(),
        name_string_t::guarantee_valid("_debug_table_status"),
        std::make_pair(debug_table_status_backend.get(), debug_table_status_backend.get()));
}
