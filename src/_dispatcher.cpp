#include <tuple>
#include "class1.hpp"
#include "class2.hpp"


#define EXEC_ACTION(CLASS_NAME, ACTION_NAME)                                                                  \
    case eosio::hash_name(#ACTION_NAME):                                                                      \
        executed = eosio::execute_action(eosio::name(receiver), eosio::name(code), &CLASS_NAME::ACTION_NAME); \
        break;

namespace contract_name
{
    static constexpr auto contract_account = "contractacc"_n;

    namespace actions
    {
        using contract_c1 = class1_contract;
        using contract_c2 = class2_contract;

        using sayhi = eosio::action_wrapper<"sayhi"_h, &contract_c1::sayhi, contract_account>;
        using sayhialice = eosio::action_wrapper<"sayhialice"_h, &contract_c1::sayhialice, contract_account>;

        using action1 = eosio::action_wrapper<"action1"_h, &contract_c2::action1, contract_account>;
        using action2 = eosio::action_wrapper<"action2"_h, &contract_c2::action2, contract_account>;

        template <typename F>
        void for_each_action(F&& f)
        {
            f("sayhi"_h, eosio::action_type_wrapper<&contract_c1::sayhi>{}, sayhi_ricardian);
            f("sayhialice"_h, eosio::action_type_wrapper<&contract_c1::sayhialice>{}, sayhialice_ricardian, "someone");

            f("action1"_h, eosio::action_type_wrapper<&contract_c2::action1>{}, action1_ricardian);
            f("action2"_h, eosio::action_type_wrapper<&contract_c2::action2>{}, action2_ricardian, "someone");
        }

        class theDispatcher {
        public:
            using func_t = std::function<bool(uint64_t, uint64_t)>;

            static theDispatcher& instance()
            {
                static theDispatcher d;
                return d;
            }

            template<typename ActionType>
            void addAction(const std::string_view& actionName, ActionType action)
            {
                auto executor = [&](auto func) { 
                    return func_t([func](const uint64_t& receiver, const uint64_t& code) { 
                        return eosio::execute_action(eosio::name(receiver), eosio::name(code), func); 
                    }); 
                };

                actions[eosio::hash_name(actionName)] = executor(action);
            }

            std::map<uint64_t, func_t> actions;

        private:
            theDispatcher() = default;
        };        


        inline bool eosio_apply(uint64_t receiver, uint64_t code, uint64_t action) {
            bool executed = false;

            auto d = theDispatcher::instance();
            d.addAction("sayhi", &class1_contract::sayhi);
            d.addAction("sayhialice", &class1_contract::sayhialice);
            d.addAction("action1", &class2_contract::action1);
            d.addAction("action2", &class2_contract::action2);

            if (code == receiver) {
                if (d.actions.count(action) > 0) {
                    executed = d.actions[action](receiver, code);
                }
                eosio::check(executed == true, "unknown action");
            }
            return executed;
        }

        // Old apply function: 
        // inline bool eosio_apply(uint64_t receiver, uint64_t code, uint64_t action)
        // {
        //     bool executed = false;
        //     if (code == receiver)
        //     {
        //         switch (action)
        //         {
        //             EXEC_ACTION(class1_contract, sayhi)
        //             EXEC_ACTION(class1_contract, sayhialice)
        //             EXEC_ACTION(class2_contract, action1)
        //             EXEC_ACTION(class2_contract, action2)
        //         }
        //         eosio::check(executed == true, "unknown action");
        //     }
        //     return executed;
        // }

    }  // namespace actions

}  // namespace contract_name


// Final part of the dispatcher
// EOSIO_ACTION_DISPATCHER(contract_name::actions)
// Expands to: 
extern "C"
{
    void __wasm_call_ctors();
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        __wasm_call_ctors();
        contract_name::actions::eosio_apply(receiver, code, action);
    }
}

// Things to populate the ABI with
EOSIO_ABIGEN(actions(contract_name::actions),
             table("schema1"_n, contract_name::Schema1),
             ricardian_clause("Class 1 clause", contract_name::ricardian_clause),
             ricardian_clause("Class 2 clause", contract_name::ricardian_clause2))




