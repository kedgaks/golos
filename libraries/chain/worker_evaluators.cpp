#include <golos/protocol/worker_operations.hpp>
#include <golos/protocol/exceptions.hpp>
#include <golos/chain/steem_evaluator.hpp>
#include <golos/chain/database.hpp>
#include <golos/chain/steem_objects.hpp>
#include <golos/chain/worker_objects.hpp>

namespace golos { namespace chain {

    void worker_proposal_evaluator::do_apply(const worker_proposal_operation& o) {
        ASSERT_REQ_HF(STEEMIT_HARDFORK_0_21__1013, "worker_proposal_operation");

        const auto& post = _db.get_comment(o.author, o.permlink);

        GOLOS_CHECK_LOGIC(post.parent_author == STEEMIT_ROOT_POST_PARENT,
            logic_exception::worker_proposal_can_be_created_only_on_post,
            "Worker proposal can be created only on post");

        const auto* wpo = _db.find_worker_proposal(post.id);

        if (wpo) {
            GOLOS_CHECK_LOGIC(wpo->state == worker_proposal_state::created,
                logic_exception::cannot_edit_worker_proposal_with_approved_techspec,
                "Cannot edit worker proposal with approved techspec");

            _db.modify(*wpo, [&](worker_proposal_object& wpo) {
                wpo.type = o.type;
            });
            return;
        }

        _db.create<worker_proposal_object>([&](worker_proposal_object& wpo) {
            wpo.author = o.author;
            wpo.post = post.id;
            wpo.type = o.type;
            wpo.state = worker_proposal_state::created;
        });
    }

    void worker_proposal_delete_evaluator::do_apply(const worker_proposal_delete_operation& o) {
        ASSERT_REQ_HF(STEEMIT_HARDFORK_0_21__1013, "worker_proposal_delete_operation");

        const auto& post = _db.get_comment(o.author, o.permlink);

        const auto& wpo = _db.get_worker_proposal(post.id);

        GOLOS_CHECK_LOGIC(wpo.state == worker_proposal_state::created,
            logic_exception::cannot_delete_worker_proposal_with_approved_techspec,
            "Cannot delete worker proposal with approved techspec");

        const auto& wto_idx = _db.get_index<worker_techspec_index, by_worker_proposal>();
        auto wto_itr = wto_idx.find(std::make_tuple(post.author, post.permlink));
        GOLOS_CHECK_LOGIC(wto_itr == wto_idx.end(),
            logic_exception::cannot_delete_worker_proposal_with_techspecs,
            "Cannot delete worker proposal with techspecs");

        _db.remove(wpo);
    }

    void worker_techspec_evaluator::do_apply(const worker_techspec_operation& o) {
        ASSERT_REQ_HF(STEEMIT_HARDFORK_0_21__1013, "worker_techspec_operation");

        const auto now = _db.head_block_time();

        const auto& post = _db.get_comment(o.author, o.permlink);

        GOLOS_CHECK_LOGIC(post.parent_author == STEEMIT_ROOT_POST_PARENT,
            logic_exception::worker_techspec_can_be_created_only_on_post,
            "Worker techspec can be created only on post");

        const auto& wpo_post = _db.get_comment(o.worker_proposal_author, o.worker_proposal_permlink);
        const auto* wpo = _db.find_worker_proposal(wpo_post.id);

        GOLOS_CHECK_LOGIC(wpo,
            logic_exception::worker_techspec_can_be_created_only_for_existing_proposal,
            "Worker techspec can be created only for existing proposal");

        GOLOS_CHECK_LOGIC(wpo->state == worker_proposal_state::created,
            logic_exception::this_worker_proposal_already_has_approved_techspec,
            "This worker proposal already has approved techspec");

        GOLOS_CHECK_LOGIC(wpo->type != worker_proposal_type::premade_work,
            logic_exception::cannot_create_techspec_for_premade_worker_proposal,
            "Cannot create techspec for premade worker proposal");

        const auto& wto_idx = _db.get_index<worker_techspec_index, by_worker_proposal>();
        auto wto_itr = wto_idx.find(std::make_tuple(wpo_post.author, wpo_post.permlink, o.author));
        if (wto_itr != wto_idx.end()) {
            GOLOS_CHECK_LOGIC(o.permlink == to_string(post.permlink),
                logic_exception::there_already_is_your_techspec_with_another_permlink,
                "There already is your techspec with another permlink");

            GOLOS_CHECK_LOGIC(o.specification_cost.symbol == wto_itr->specification_cost.symbol,
                logic_exception::cannot_change_cost_symbol,
                "Cannot change cost symbol");
            GOLOS_CHECK_LOGIC(o.development_cost.symbol == wto_itr->development_cost.symbol,
                logic_exception::cannot_change_cost_symbol,
                "Cannot change cost symbol");

            _db.modify(*wto_itr, [&](worker_techspec_object& wto) {
                wto.specification_cost = o.specification_cost;
                wto.development_cost = o.development_cost;
                wto.payments_count = o.payments_count;
                wto.payments_interval = o.payments_interval;
            });

            return;
        }

        _db.create<worker_techspec_object>([&](worker_techspec_object& wto) {
            wto.author = o.author;
            wto.post = post.id;
            wto.worker_proposal_author = o.worker_proposal_author;
            from_string(wto.worker_proposal_permlink, o.worker_proposal_permlink);
            wto.state = worker_techspec_state::created;
            wto.created = now;
            wto.specification_cost = o.specification_cost;
            wto.development_cost = o.development_cost;
            wto.payments_count = o.payments_count;
            wto.payments_interval = o.payments_interval;
        });
    }

    void worker_techspec_delete_evaluator::do_apply(const worker_techspec_delete_operation& o) {
        ASSERT_REQ_HF(STEEMIT_HARDFORK_0_21__1013, "worker_techspec_delete_operation");

        const auto& post = _db.get_comment(o.author, o.permlink);
        const auto& wto = _db.get_worker_techspec(post.id);

        GOLOS_CHECK_LOGIC(wto.state == worker_techspec_state::payment || wto.state == worker_techspec_state::payment_complete,
            logic_exception::cannot_delete_paying_worker_techspec,
            "Cannot delete paying worker techspec");

        if (wto.state == worker_techspec_state::approved) {
            const auto& wpo_post = _db.get_comment(wto.worker_proposal_author, wto.worker_proposal_permlink);
            const auto& wpo = _db.get_worker_proposal(wpo_post.id);
            _db.modify(wpo, [&](worker_proposal_object& wpo) {
                wpo.state = worker_proposal_state::created;
            });
        }

        _db.remove(wto);
    }

    void worker_techspec_approve_evaluator::do_apply(const worker_techspec_approve_operation& o) {
        ASSERT_REQ_HF(STEEMIT_HARDFORK_0_21__1013, "worker_techspec_approve_operation");

        auto approver_witness = _db.get_witness(o.approver);
        GOLOS_CHECK_LOGIC(approver_witness.schedule == witness_object::top19,
            logic_exception::approver_of_techspec_should_be_in_top19_of_witnesses,
            "Approver of techspec should be in Top 19 of witnesses");

        const auto& wto_post = _db.get_comment(o.author, o.permlink);
        const auto& wto = _db.get_worker_techspec(wto_post.id);

        const auto& wpo_post = _db.get_comment(wto.worker_proposal_author, wto.worker_proposal_permlink);
        const auto& wpo = _db.get_worker_proposal(wpo_post.id);

        GOLOS_CHECK_LOGIC(wpo.state == worker_proposal_state::created,
            logic_exception::this_worker_proposal_already_has_approved_techspec,
            "This worker proposal already has approved techspec");

        GOLOS_CHECK_LOGIC(wto.state == worker_techspec_state::created,
            logic_exception::techspec_is_already_approved_or_closed,
            "Techspec is already approved or closed");

        const auto& wtao_idx = _db.get_index<worker_techspec_approve_index, by_techspec_approver>();
        auto wtao_itr = wtao_idx.find(std::make_tuple(wto.post, o.approver));

        if (o.state == worker_techspec_approve_state::abstain) {
            if (wtao_itr != wtao_idx.end()) {
                _db.remove(*wtao_itr);
            }

            return;
        }

        if (wtao_itr != wtao_idx.end()) {
            _db.modify(*wtao_itr, [&](worker_techspec_approve_object& wtao) {
                wtao.state = o.state;
            });
        } else {
            _db.create<worker_techspec_approve_object>([&](worker_techspec_approve_object& wtao) {
                wtao.approver = o.approver;
                wtao.post = wto.post;
                wtao.state = o.state;
            });
        }

        auto count_approvers = [&](auto state) {
            auto approvers = 0;
            wtao_itr = wtao_idx.lower_bound(wto.post);
            for (; wtao_itr != wtao_idx.end() && wtao_itr->post == wto.post; ++wtao_itr) {
                auto witness = _db.find_witness(wtao_itr->approver);
                if (witness && witness->schedule == witness_object::top19 && wtao_itr->state == state) {
                    approvers++;
                }
            }
            return approvers;
        };

        if (o.state == worker_techspec_approve_state::disapprove) {
            auto disapprovers = count_approvers(worker_techspec_approve_state::disapprove);

            if (disapprovers < STEEMIT_SUPER_MAJOR_VOTED_WITNESSES) {
                return;
            }

            _db.clear_worker_techspec_approves(wto);

            _db.modify(wto, [&](worker_techspec_object& wto) {
                wto.state = worker_techspec_state::closed;
            });
        } else if (o.state == worker_techspec_approve_state::approve) {
            auto approvers = count_approvers(worker_techspec_approve_state::approve);

            if (approvers < STEEMIT_MAJOR_VOTED_WITNESSES) {
                return;
            }

            _db.modify(wpo, [&](worker_proposal_object& wpo) {
                wpo.approved_techspec_author = o.author;
                from_string(wpo.approved_techspec_permlink, o.permlink);
                wpo.state = worker_proposal_state::techspec;
            });

            _db.clear_worker_techspec_approves(wto);

            _db.modify(wto, [&](worker_techspec_object& wto) {
                wto.state = worker_techspec_state::approved;
            });
        }
    }

    void worker_result_check_post(const database& _db, const comment_object& post) {
        GOLOS_CHECK_LOGIC(post.parent_author == STEEMIT_ROOT_POST_PARENT,
            logic_exception::worker_result_can_be_created_only_on_post,
            "Worker result can be created only on post");

        const auto* wto_result = _db.find_worker_result(post.author, post.permlink);
        GOLOS_CHECK_LOGIC(!wto_result,
            logic_exception::this_post_already_used_as_worker_result,
            "This post already used as worker result");

        const auto* wto = _db.find_worker_techspec(post.id);
        GOLOS_CHECK_LOGIC(!wto,
            logic_exception::this_post_already_used_as_worker_techspec,
            "This post already used as worker techspec");
    }

    void worker_result_evaluator::do_apply(const worker_result_operation& o) {
        ASSERT_REQ_HF(STEEMIT_HARDFORK_0_21__1013, "worker_result_operation");

        const auto now = _db.head_block_time();

        GOLOS_CHECK_LOGIC(o.completion_date <= now,
            logic_exception::work_completion_date_cannot_be_in_future,
            "Work completion date cannot be in future");

        const auto& post = _db.get_comment(o.author, o.permlink);

        worker_result_check_post(_db, post);

        const auto& wto_post = _db.get_comment(o.author, o.worker_techspec_permlink);
        const auto& wto = _db.get_worker_techspec(wto_post.id);

        const auto& wpo_post = _db.get_comment(wto.worker_proposal_author, wto.worker_proposal_permlink);
        const auto& wpo = _db.get_worker_proposal(wpo_post.id);

        GOLOS_CHECK_LOGIC(wpo.type != worker_proposal_type::premade_work,
            logic_exception::only_premade_worker_result_can_be_created_for_premade_worker_proposal,
            "Only premade worker result can be created for premade worker proposal");

        GOLOS_CHECK_LOGIC(wto.state == worker_techspec_state::work,
            logic_exception::worker_result_can_be_created_only_for_techspec_in_work,
            "Worker result can be created only for techspec in work");

        _db.modify(wto, [&](worker_techspec_object& wto) {
            from_string(wto.worker_result_permlink, o.permlink);

            if (o.completion_date != time_point_sec::min()) {
                wto.completion_date = o.completion_date;
            } else {
                wto.completion_date = now;
            }

            wto.state = worker_techspec_state::complete;
        });
    }

    void worker_result_premade_evaluator::do_apply(const worker_result_premade_operation& o) {
        ASSERT_REQ_HF(STEEMIT_HARDFORK_0_21__1013, "worker_result_premade_operation");

        const auto now = _db.head_block_time();

        GOLOS_CHECK_LOGIC(o.completion_date <= now,
            logic_exception::work_completion_date_cannot_be_in_future,
            "Work completion date cannot be in future");

        const auto& post = _db.get_comment(o.author, o.permlink);

        worker_result_check_post(_db, post);

        const auto& wpo_post = _db.get_comment(o.worker_proposal_author, o.worker_proposal_permlink);
        const auto& wpo = _db.get_worker_proposal(wpo_post.id);

        GOLOS_CHECK_LOGIC(wpo.type == worker_proposal_type::premade_work,
            logic_exception::premade_result_can_be_created_only_for_premade_work_proposal,
            "Premade result can be created only for premade work proposal");

        GOLOS_CHECK_LOGIC(wpo.state == worker_proposal_state::created,
            logic_exception::this_worker_proposal_already_has_approved_techspec,
            "This worker proposal already has approved techspec");

        _db.create<worker_techspec_object>([&](worker_techspec_object& wto) {
            wto.author = o.author;
            wto.post = post.id;
            wto.worker_proposal_author = o.worker_proposal_author;
            from_string(wto.worker_proposal_permlink, o.worker_proposal_permlink);
            wto.created = now;
            wto.worker = o.author;
            wto.specification_cost = o.specification_cost;
            wto.development_cost = o.development_cost;
            wto.payments_count = o.payments_count;
            wto.payments_interval = o.payments_interval;

            from_string(wto.worker_result_permlink, o.permlink);

            if (o.completion_date != time_point_sec::min()) {
                wto.completion_date = o.completion_date;
            } else {
                wto.completion_date = now;
            }

            wto.state = worker_techspec_state::complete;
        });
    }

    void worker_result_delete_evaluator::do_apply(const worker_result_delete_operation& o) {
        ASSERT_REQ_HF(STEEMIT_HARDFORK_0_21__1013, "worker_result_delete_operation");

        const auto& wto = _db.get_worker_result(o.author, o.permlink);

        GOLOS_CHECK_LOGIC(wto.state == worker_techspec_state::payment || wto.state == worker_techspec_state::payment_complete,
            logic_exception::cannot_delete_worker_result_for_paying_techspec,
            "Cannot delete worker result for paying techspec");

        _db.modify(wto, [&](worker_techspec_object& wto) {
            wto.worker_result_permlink.clear();
            wto.completion_date = time_point::min();
            wto.state = worker_techspec_state::work;
        });
    }

    void worker_result_approve_evaluator::do_apply(const worker_result_approve_operation& o) {
        ASSERT_REQ_HF(STEEMIT_HARDFORK_0_21__1013, "worker_result_approve_operation");

        auto approver_witness = _db.get_witness(o.approver);
        GOLOS_CHECK_LOGIC(approver_witness.schedule == witness_object::top19,
            logic_exception::approver_of_result_should_be_in_top19_of_witnesses,
            "Approver of result should be in Top 19 of witnesses");

        const auto& wto_post = _db.get_comment(o.author, o.permlink);
        const auto& wto = _db.get_worker_result(o.author, o.permlink);

        const auto& wpo_post = _db.get_comment(wto.worker_proposal_author, wto.worker_proposal_permlink);
        const auto& wpo = _db.get_worker_proposal(wpo_post.id);

        if (wpo.type == worker_proposal_type::premade_work) {
            GOLOS_CHECK_LOGIC(wpo.state == worker_proposal_state::created,
                logic_exception::this_worker_proposal_already_has_approved_result,
                "This worker proposal already has approved result");
        }

        GOLOS_CHECK_LOGIC(wto.state == worker_techspec_state::complete,
            logic_exception::worker_techspec_should_be_complete_to_approve_result,
            "Worker techspec should be complete to approve result");

        const auto& mprops = _db.get_witness_schedule_object().median_props;
        GOLOS_CHECK_LOGIC(_db.head_block_time() <= wto.completion_date + mprops.worker_result_approve_term_sec,
            logic_exception::approve_term_has_expired,
            "Approve term has expired");

        const auto& wrao_idx = _db.get_index<worker_result_approve_index, by_result_approver>();
        auto wrao_itr = wrao_idx.find(std::make_tuple(wto_post.id, o.approver));

        if (o.state == worker_techspec_approve_state::abstain) {
            if (wrao_itr != wrao_idx.end()) {
                _db.remove(*wrao_itr);
            }

            return;
        }

        if (wrao_itr != wrao_idx.end()) {
            _db.modify(*wrao_itr, [&](worker_result_approve_object& wrao) {
                wrao.state = o.state;
            });
        } else {
            _db.create<worker_result_approve_object>([&](worker_result_approve_object& wrao) {
                wrao.approver = o.approver;
                wrao.post = wto.post;
                wrao.state = o.state;
            });
        }

        auto count_approvers = [&](auto state) {
            auto approvers = 0;
            wrao_itr = wrao_idx.lower_bound(wto.post);
            for (; wrao_itr != wrao_idx.end() && wrao_itr->post == wto.post; ++wrao_itr) {
                auto witness = _db.find_witness(wrao_itr->approver);
                if (witness && witness->schedule == witness_object::top19 && wrao_itr->state == state) {
                    approvers++;
                }
            }
            return approvers;
        };

        const auto& gpo = _db.get_dynamic_global_properties();

        if (o.state == worker_techspec_approve_state::disapprove) {
            auto disapprovers = count_approvers(worker_techspec_approve_state::disapprove);

            if (disapprovers < STEEMIT_SUPER_MAJOR_VOTED_WITNESSES) {
                return;
            }

            _db.modify(wto, [&](worker_techspec_object& wto) {
                if (wpo.type == worker_proposal_type::premade_work) {
                    wto.state = worker_techspec_state::closed;
                } else {
                    wto.state = worker_techspec_state::work;
                }
            });
        } else if (o.state == worker_techspec_approve_state::approve) {
            auto month_sec = fc::days(30).to_seconds();
            auto payments_period = int64_t(wto.payments_interval) * wto.payments_count;

            auto consumption = _db.calculate_worker_techspec_month_consumption(wto);

            uint128_t revenue_funds(gpo.worker_revenue_per_month.amount.value);
            revenue_funds = revenue_funds * payments_period / month_sec;
            revenue_funds += gpo.total_worker_fund_steem.amount.value;

            auto consumption_funds = uint128_t(gpo.worker_consumption_per_month.amount.value) + consumption.amount.value;
            consumption_funds = consumption_funds * payments_period / month_sec;

            GOLOS_CHECK_LOGIC(revenue_funds >= consumption_funds,
                logic_exception::insufficient_funds_to_approve_worker_result,
                "Insufficient funds to approve worker result");

            auto approvers = count_approvers(worker_techspec_approve_state::approve);

            if (approvers < STEEMIT_MAJOR_VOTED_WITNESSES) {
                return;
            }

            _db.modify(wto, [&](worker_techspec_object& wto) {
                wto.next_cashout_time = _db.head_block_time() + wto.payments_interval;
                wto.state = worker_techspec_state::payment;
            });
            
            if (wpo.type == worker_proposal_type::premade_work) {
                _db.modify(wpo, [&](worker_proposal_object& wpo) {
                    wpo.state = worker_proposal_state::techspec;
                });
            }

            _db.modify(gpo, [&](dynamic_global_property_object& gpo) {
                gpo.worker_consumption_per_month += consumption;
            });
        }
    }

    void worker_assign_evaluator::do_apply(const worker_assign_operation& o) {
        ASSERT_REQ_HF(STEEMIT_HARDFORK_0_21__1013, "worker_assign_operation");

        _db.get_account(o.worker);

        const auto& wto_post = _db.get_comment(o.worker_techspec_author, o.worker_techspec_permlink);
        const auto& wto = _db.get_worker_techspec(wto_post.id);

        if (!o.worker.size()) { // Unassign worker
            GOLOS_CHECK_LOGIC(wto.state == worker_techspec_state::work,
                logic_exception::cannot_unassign_worker_from_finished_or_not_started_work,
                "Cannot unassign worker from finished or not started work");

            GOLOS_CHECK_LOGIC(o.assigner == wto.worker || o.assigner == wto.author,
                logic_exception::worker_can_be_unassigned_only_by_techspec_author_or_himself,
                "Worker can be unassigned only by techspec author or himself");

            _db.modify(wto, [&](worker_techspec_object& wto) {
                wto.worker = account_name_type();
                wto.state = worker_techspec_state::approved;
            });

            return;
        }

        GOLOS_CHECK_LOGIC(wto.state == worker_techspec_state::approved,
            logic_exception::worker_can_be_assigned_only_to_proposal_with_approved_techspec,
            "Worker can be assigned only to proposal with approved techspec");

        const auto& wpo_post = _db.get_comment(wto.worker_proposal_author, wto.worker_proposal_permlink);
        const auto& wpo = _db.get_worker_proposal(wpo_post.id);
        GOLOS_CHECK_LOGIC(wpo.type == worker_proposal_type::task,
            logic_exception::worker_cannot_be_assigned_to_premade_proposal,
            "Worker cannot be assigned to premade proposal");

        _db.modify(wto, [&](worker_techspec_object& wto) {
            wto.worker = o.worker;
            wto.state = worker_techspec_state::work;
        });
    }

} } // golos::chain
