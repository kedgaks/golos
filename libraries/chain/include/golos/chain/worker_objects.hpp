#pragma once

#include <golos/chain/database.hpp>

namespace golos { namespace chain {

    enum class worker_proposal_state {
        created,
        techspec
    };

    class worker_proposal_object : public object<worker_proposal_object_type, worker_proposal_object> {
    public:
        worker_proposal_object() = delete;

        template <typename Constructor, typename Allocator>
        worker_proposal_object(Constructor&& c, allocator <Allocator> a)
            : approved_techspec_permlink(a) {
            c(*this);
        };

        id_type id;

        account_name_type author;
        comment_id_type post;
        worker_proposal_type type;
        worker_proposal_state state;
        account_name_type approved_techspec_author;
        shared_string approved_techspec_permlink;
    };

    enum class worker_techspec_state {
        created,
        approved,
        work,
        complete,
        payment,
        payment_complete,
        closed
    };

    class worker_techspec_object : public object<worker_techspec_object_type, worker_techspec_object> {
    public:
        worker_techspec_object() = delete;

        template <typename Constructor, typename Allocator>
        worker_techspec_object(Constructor&& c, allocator <Allocator> a)
            : worker_proposal_permlink(a), worker_result_permlink(a) {
            c(*this);
        };

        id_type id;

        account_name_type author;
        comment_id_type post;
        account_name_type worker_proposal_author;
        shared_string worker_proposal_permlink;
        worker_techspec_state state;
        time_point_sec created;
        asset specification_cost;
        asset development_cost;
        account_name_type worker;
        shared_string worker_result_permlink;
        time_point_sec completion_date;
        uint16_t payments_count;
        uint32_t payments_interval;
        time_point_sec next_cashout_time = time_point_sec::maximum();
        uint16_t finished_payments_count = 0;
    };

    class worker_techspec_approve_object : public object<worker_techspec_approve_object_type, worker_techspec_approve_object> {
    public:
        worker_techspec_approve_object() = delete;

        template <typename Constructor, typename Allocator>
        worker_techspec_approve_object(Constructor&& c, allocator <Allocator> a) {
            c(*this);
        };

        id_type id;

        account_name_type approver;
        comment_id_type post;
        worker_techspec_approve_state state;
    };

    class worker_result_approve_object : public object<worker_result_approve_object_type, worker_result_approve_object> {
    public:
        worker_result_approve_object() = delete;

        template <typename Constructor, typename Allocator>
        worker_result_approve_object(Constructor&& c, allocator <Allocator> a) {
            c(*this);
        };

        id_type id;

        account_name_type approver;
        comment_id_type post;
        worker_techspec_approve_state state;
    };

    class worker_payment_approve_object : public object<worker_payment_approve_object_type, worker_payment_approve_object> {
    public:
        worker_payment_approve_object() = delete;

        template <typename Constructor, typename Allocator>
        worker_payment_approve_object(Constructor&& c, allocator <Allocator> a) {
            c(*this);
        };

        id_type id;

        account_name_type approver;
        comment_id_type post;
        worker_techspec_approve_state state;
    };

    struct by_post;

    using worker_proposal_index = multi_index_container<
        worker_proposal_object,
        indexed_by<
            ordered_unique<
                tag<by_id>,
                member<worker_proposal_object, worker_proposal_object_id_type, &worker_proposal_object::id>>,
            ordered_unique<
                tag<by_post>,
                member<worker_proposal_object, comment_id_type, &worker_proposal_object::post>>>,
        allocator<worker_proposal_object>>;

    struct by_worker_proposal;
    struct by_worker_result;
    struct by_next_cashout_time;
    struct by_created;

    using worker_techspec_index = multi_index_container<
        worker_techspec_object,
        indexed_by<
            ordered_unique<
                tag<by_id>,
                member<worker_techspec_object, worker_techspec_object_id_type, &worker_techspec_object::id>>,
            ordered_unique<
                tag<by_post>,
                member<worker_techspec_object, comment_id_type, &worker_techspec_object::post>>,
            ordered_non_unique<
                tag<by_worker_proposal>,
                composite_key<
                    worker_techspec_object,
                    member<worker_techspec_object, account_name_type, &worker_techspec_object::worker_proposal_author>,
                    member<worker_techspec_object, shared_string, &worker_techspec_object::worker_proposal_permlink>,
                    member<worker_techspec_object, account_name_type, &worker_techspec_object::author>>,
                composite_key_compare<
                    std::less<account_name_type>,
                    chainbase::strcmp_less,
                    std::less<account_name_type>>>,
            ordered_unique<
                tag<by_worker_result>,
                composite_key<
                    worker_techspec_object,
                    member<worker_techspec_object, account_name_type, &worker_techspec_object::author>,
                    member<worker_techspec_object, shared_string, &worker_techspec_object::worker_result_permlink>>,
                composite_key_compare<
                    std::less<account_name_type>,
                    chainbase::strcmp_less>>,
            ordered_unique<
                tag<by_next_cashout_time>,
                composite_key<
                    worker_techspec_object,
                    member<worker_techspec_object, time_point_sec, &worker_techspec_object::next_cashout_time>,
                    member<worker_techspec_object, worker_techspec_object_id_type, &worker_techspec_object::id>>>,
            ordered_unique<
                tag<by_created>,
                composite_key<
                    worker_techspec_object,
                    member<worker_techspec_object, time_point_sec, &worker_techspec_object::created>,
                    member<worker_techspec_object, worker_techspec_object_id_type, &worker_techspec_object::id>>>>,
        allocator<worker_techspec_object>>;

    struct by_techspec_approver;

    using worker_techspec_approve_index = multi_index_container<
        worker_techspec_approve_object,
        indexed_by<
            ordered_unique<
                tag<by_id>,
                member<worker_techspec_approve_object, worker_techspec_approve_object_id_type, &worker_techspec_approve_object::id>>,
            ordered_unique<
                tag<by_techspec_approver>,
                composite_key<
                    worker_techspec_approve_object,
                    member<worker_techspec_approve_object, comment_id_type, &worker_techspec_approve_object::post>,
                    member<worker_techspec_approve_object, account_name_type, &worker_techspec_approve_object::approver>>,
                composite_key_compare<
                    std::less<comment_id_type>,
                    std::less<account_name_type>>>>,
        allocator<worker_techspec_approve_object>>;

    struct by_result_approver;

    using worker_result_approve_index = multi_index_container<
        worker_result_approve_object,
        indexed_by<
            ordered_unique<
                tag<by_id>,
                member<worker_result_approve_object, worker_result_approve_object_id_type, &worker_result_approve_object::id>>,
            ordered_unique<
                tag<by_result_approver>,
                composite_key<
                    worker_result_approve_object,
                    member<worker_result_approve_object, comment_id_type, &worker_result_approve_object::post>,
                    member<worker_result_approve_object, account_name_type, &worker_result_approve_object::approver>>,
                composite_key_compare<
                    std::less<comment_id_type>,
                    std::less<account_name_type>>>>,
        allocator<worker_result_approve_object>>;

    using worker_payment_approve_index = multi_index_container<
        worker_payment_approve_object,
        indexed_by<
            ordered_unique<
                tag<by_id>,
                member<worker_payment_approve_object, worker_payment_approve_object_id_type, &worker_payment_approve_object::id>>,
            ordered_unique<
                tag<by_result_approver>,
                composite_key<
                    worker_payment_approve_object,
                    member<worker_payment_approve_object, comment_id_type, &worker_payment_approve_object::post>,
                    member<worker_payment_approve_object, account_name_type, &worker_payment_approve_object::approver>>,
                composite_key_compare<
                    std::less<comment_id_type>,
                    std::less<account_name_type>>>>,
        allocator<worker_payment_approve_object>>;

} } // golos::chain

FC_REFLECT_ENUM(golos::chain::worker_proposal_state, (created)(techspec))

CHAINBASE_SET_INDEX_TYPE(
    golos::chain::worker_proposal_object,
    golos::chain::worker_proposal_index);

FC_REFLECT_ENUM(golos::chain::worker_techspec_state, (created)(approved)(work)(complete)(payment)(payment_complete)(closed))

CHAINBASE_SET_INDEX_TYPE(
    golos::chain::worker_techspec_object,
    golos::chain::worker_techspec_index);

CHAINBASE_SET_INDEX_TYPE(
    golos::chain::worker_techspec_approve_object,
    golos::chain::worker_techspec_approve_index);

CHAINBASE_SET_INDEX_TYPE(
    golos::chain::worker_result_approve_object,
    golos::chain::worker_result_approve_index);

CHAINBASE_SET_INDEX_TYPE(
    golos::chain::worker_payment_approve_object,
    golos::chain::worker_payment_approve_index);
