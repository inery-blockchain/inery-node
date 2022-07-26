#pragma once

#include <inery/chain/types.hpp>
#include <inery/chain/contract_types.hpp>

namespace inery { namespace chain {

   class apply_context;

   /**
    * @defgroup native_action_handlers Native Action Handlers
    */
   ///@{
   void apply_inery_newaccount(apply_context&);
   void apply_inery_updateauth(apply_context&);
   void apply_inery_deleteauth(apply_context&);
   void apply_inery_linkauth(apply_context&);
   void apply_inery_unlinkauth(apply_context&);

   /*
   void apply_inery_postrecovery(apply_context&);
   void apply_inery_passrecovery(apply_context&);
   void apply_inery_vetorecovery(apply_context&);
   */

   void apply_inery_setcode(apply_context&);
   void apply_inery_setabi(apply_context&);

   void apply_inery_canceldelay(apply_context&);
   ///@}  end action handlers

} } /// namespace inery::chain
