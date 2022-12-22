#pragma once
#include <inery/chain/block_timestamp.hpp>
#include <inery/chain/master_schedule.hpp>
#include <inery/chain/protocol_feature_activation.hpp>

#include <type_traits>

namespace inery { namespace chain {

   namespace detail {
      template<typename... Ts>
      struct block_header_extension_types {
         using block_header_extension_t = fc::static_variant< Ts... >;
         using decompose_t = decompose< Ts... >;
      };
   }

   using block_header_extension_types = detail::block_header_extension_types<
      protocol_feature_activation,
      master_schedule_change_extension
   >;

   using block_header_extension = block_header_extension_types::block_header_extension_t;

   struct block_header
   {
      block_timestamp_type             timestamp;
      account_name                     master;

      /**
       *  By signing this block this master is confirming blocks [block_num() - confirmed, blocknum())
       *  as being the best blocks for that range and that he has not signed any other
       *  statements that would contradict.
       *
       *  No master should sign a block with overlapping ranges or it is proof of byzantine
       *  behavior. When producing a block a master is always confirming at least the block he
       *  is building off of.  A master cannot confirm "this" block, only prior blocks.
       */
      uint16_t                         confirmed = 1;

      block_id_type                    previous;

      checksum256_type                 transaction_mroot; /// mroot of cycles_summary
      checksum256_type                 action_mroot; /// mroot of all delivered action receipts

      /**
       * LEGACY SUPPORT - After enabling the wtmsig-blocks extension this field is deprecated and must be empty
       *
       * Prior to that activation this carries:
       *
       * The master schedule version that should validate this block, this is used to
       * indicate that the prior block which included new_masters->version has been marked
       * irreversible and that it the new master schedule takes effect this block.
       */

      using new_masters_type = optional<legacy::master_schedule_type>;

      uint32_t                            schedule_version = 0;
      new_masters_type                    new_masters;
      extensions_type                     header_extensions;


      block_header() = default;

      digest_type       digest()const;
      block_id_type     id() const;
      uint32_t          block_num() const { return num_from_id(previous) + 1; }
      static uint32_t   num_from_id(const block_id_type& id);

      flat_multimap<uint16_t, block_header_extension> validate_and_extract_header_extensions()const;
   };


   struct signed_block_header : public block_header
   {
      signature_type    producer_signature;
   };

} } /// namespace inery::chain

FC_REFLECT(inery::chain::block_header,
           (timestamp)(master)(confirmed)(previous)
           (transaction_mroot)(action_mroot)
           (schedule_version)(new_masters)(header_extensions))

FC_REFLECT_DERIVED(inery::chain::signed_block_header, (inery::chain::block_header), (producer_signature))
