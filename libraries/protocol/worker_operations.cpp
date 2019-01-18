#include <golos/protocol/worker_operations.hpp>
#include <golos/protocol/exceptions.hpp>
#include <golos/protocol/validate_helper.hpp>

namespace golos { namespace protocol {

    void worker_proposal_operation::validate() const {
        GOLOS_CHECK_PARAM_ACCOUNT(author);
        GOLOS_CHECK_PARAM(permlink, validate_permlink(permlink));

        GOLOS_CHECK_PARAM(type, {
            GOLOS_CHECK_VALUE(type < worker_proposal_type::_size, "This value is reserved");
        });
    }

    void worker_proposal_delete_operation::validate() const {
        GOLOS_CHECK_PARAM_ACCOUNT(author);
        GOLOS_CHECK_PARAM(permlink, validate_permlink(permlink));
    }

    void worker_techspec_operation::validate() const {
        GOLOS_CHECK_PARAM_ACCOUNT(author);
        GOLOS_CHECK_PARAM(permlink, validate_permlink(permlink));
        GOLOS_CHECK_PARAM_ACCOUNT(worker_proposal_author);
        GOLOS_CHECK_PARAM(worker_proposal_permlink, validate_permlink(worker_proposal_permlink));

        GOLOS_CHECK_PARAM(specification_cost, {
            GOLOS_CHECK_ASSET_GOLOS(specification_cost);
            GOLOS_CHECK_VALUE_GE(specification_cost.amount, 0);
        });
        GOLOS_CHECK_PARAM(development_cost, {
            GOLOS_CHECK_ASSET_GOLOS(development_cost);
            GOLOS_CHECK_VALUE_GE(development_cost.amount, 0);
        });

        GOLOS_CHECK_PARAM(payments_count, {
            GOLOS_CHECK_VALUE_GE(payments_count, 1);
        });
        GOLOS_CHECK_PARAM(payments_interval, {
            if (payments_count == 1) {
                GOLOS_CHECK_VALUE_EQ(payments_interval, 0);
                return;
            }
            GOLOS_CHECK_VALUE_GT(payments_interval, 0);
            GOLOS_CHECK_VALUE_LE(payments_interval * payments_count, development_eta);
        });
    }

    void worker_techspec_delete_operation::validate() const {
        GOLOS_CHECK_PARAM_ACCOUNT(author);
        GOLOS_CHECK_PARAM(permlink, validate_permlink(permlink));
    }

    void worker_techspec_approve_operation::validate() const {
        GOLOS_CHECK_PARAM_ACCOUNT(approver);
        GOLOS_CHECK_PARAM_ACCOUNT(author);
        GOLOS_CHECK_PARAM(permlink, validate_permlink(permlink));

        GOLOS_CHECK_PARAM(state, {
            GOLOS_CHECK_VALUE(state < worker_techspec_approve_state::_size, "This value is reserved");
        });
    }

    void worker_result_fill_operation::validate() const {
        GOLOS_CHECK_PARAM_ACCOUNT(author);
        GOLOS_CHECK_PARAM(permlink, validate_permlink(permlink));
        GOLOS_CHECK_PARAM(worker_techspec_permlink, validate_permlink(worker_techspec_permlink));
    }

    void worker_result_clear_operation::validate() const {
        GOLOS_CHECK_PARAM_ACCOUNT(author);
        GOLOS_CHECK_PARAM(permlink, validate_permlink(permlink));
    }

    void worker_result_approve_operation::validate() const {
        GOLOS_CHECK_PARAM_ACCOUNT(approver);
        GOLOS_CHECK_PARAM_ACCOUNT(author);
        GOLOS_CHECK_PARAM(permlink, validate_permlink(permlink));

        GOLOS_CHECK_PARAM(state, {
            GOLOS_CHECK_VALUE(state < worker_techspec_approve_state::_size, "This value is reserved");
        });
    }

} } // golos::protocol