#pragma once
#include <inery/chain/config.hpp>
#include <inery/chain/types.hpp>
#include <chainbase/chainbase.hpp>
#include <inery/chain/authority.hpp>
#include <inery/chain/snapshot.hpp>

namespace inery { namespace chain {

   namespace legacy {
      /**
       *  Used as part of the master_schedule_type, maps the master name to their key.
       */
      struct master_key {
         account_name      master_name;
         public_key_type   block_signing_key;

         friend bool operator == ( const master_key& lhs, const master_key& rhs ) {
            return tie( lhs.master_name, lhs.block_signing_key ) == tie( rhs.master_name, rhs.block_signing_key );
         }
         friend bool operator != ( const master_key& lhs, const master_key& rhs ) {
            return tie( lhs.master_name, lhs.block_signing_key ) != tie( rhs.master_name, rhs.block_signing_key );
         }
      };

      /**
       *  Defines both the order, account name, and signing keys of the active set of masters.
       */
      struct master_schedule_type {
         uint32_t                                       version = 0; ///< sequentially incrementing version number
         vector<master_key>                              masters;

         friend bool operator == ( const master_schedule_type& a, const master_schedule_type& b )
         {
            if( a.version != b.version ) return false;
            if ( a.masters.size() != b.masters.size() ) return false;
            for( uint32_t i = 0; i < a.masters.size(); ++i )
               if( a.masters[i] != b.masters[i] ) return false;
            return true;
         }

         friend bool operator != ( const master_schedule_type& a, const master_schedule_type& b )
         {
            return !(a==b);
         }
      };
   }

   struct shared_block_signing_authority_v0 {
      shared_block_signing_authority_v0() = delete;
      shared_block_signing_authority_v0( const shared_block_signing_authority_v0& ) = default;
      shared_block_signing_authority_v0( shared_block_signing_authority_v0&& ) = default;
      shared_block_signing_authority_v0& operator= ( shared_block_signing_authority_v0 && ) = default;
      shared_block_signing_authority_v0& operator= ( const shared_block_signing_authority_v0 & ) = default;

      explicit shared_block_signing_authority_v0( chainbase::allocator<char> alloc )
      :keys(alloc){}

      uint32_t                           threshold = 0;
      shared_vector<shared_key_weight>   keys;
   };

   using shared_block_signing_authority = static_variant<shared_block_signing_authority_v0>;

   struct shared_master_authority {
      shared_master_authority() = delete;
      shared_master_authority( const shared_master_authority& ) = default;
      shared_master_authority( shared_master_authority&& ) = default;
      shared_master_authority& operator= ( shared_master_authority && ) = default;
      shared_master_authority& operator= ( const shared_master_authority & ) = default;

      shared_master_authority( const name& master_name, shared_block_signing_authority&& authority )
      :master_name(master_name)
      ,authority(std::move(authority))
      {}

      name                                     master_name;
      shared_block_signing_authority           authority;
   };

   struct shared_master_authority_schedule {
      shared_master_authority_schedule() = delete;

      explicit shared_master_authority_schedule( chainbase::allocator<char> alloc )
      :masters(alloc){}

      shared_master_authority_schedule( const shared_master_authority_schedule& ) = default;
      shared_master_authority_schedule( shared_master_authority_schedule&& ) = default;
      shared_master_authority_schedule& operator= ( shared_master_authority_schedule && ) = default;
      shared_master_authority_schedule& operator= ( const shared_master_authority_schedule & ) = default;

      uint32_t                                     version = 0; ///< sequentially incrementing version number
      shared_vector<shared_master_authority>       masters;
   };

   /**
    * block signing authority version 0
    * this authority allows for a weighted threshold multi-sig per-master
    */
   struct block_signing_authority_v0 {
      static constexpr std::string_view abi_type_name() { return "block_signing_authority_v0"; }

      uint32_t                    threshold = 0;
      vector<key_weight>          keys;

      template<typename Op>
      void for_each_key( Op&& op ) const {
         for (const auto& kw : keys ) {
            op(kw.key);
         }
      }

      std::pair<bool, size_t> keys_satisfy_and_relevant( const std::set<public_key_type>& presented_keys ) const {
         size_t num_relevant_keys = 0;
         uint32_t total_weight = 0;
         for (const auto& kw : keys ) {
            const auto& iter = presented_keys.find(kw.key);
            if (iter != presented_keys.end()) {
               ++num_relevant_keys;

               if( total_weight < threshold ) {
                  total_weight += std::min<uint32_t>(std::numeric_limits<uint32_t>::max() - total_weight, kw.weight);
               }
            }
         }

         return {total_weight >= threshold, num_relevant_keys};
      }

      auto to_shared(chainbase::allocator<char> alloc) const {
         shared_block_signing_authority_v0 result(alloc);
         result.threshold = threshold;
         result.keys.clear();
         result.keys.reserve(keys.size());
         for (const auto& k: keys) {
            result.keys.emplace_back(shared_key_weight::convert(alloc, k));
         }

         return result;
      }

      static auto from_shared(const shared_block_signing_authority_v0& src) {
         block_signing_authority_v0 result;
         result.threshold = src.threshold;
         result.keys.reserve(src.keys.size());
         for (const auto& k: src.keys) {
            result.keys.push_back(k);
         }

         return result;
      }

      friend bool operator == ( const block_signing_authority_v0& lhs, const block_signing_authority_v0& rhs ) {
         return tie( lhs.threshold, lhs.keys ) == tie( rhs.threshold, rhs.keys );
      }
      friend bool operator != ( const block_signing_authority_v0& lhs, const block_signing_authority_v0& rhs ) {
         return tie( lhs.threshold, lhs.keys ) != tie( rhs.threshold, rhs.keys );
      }
   };

   using block_signing_authority = static_variant<block_signing_authority_v0>;

   struct master_authority {
      name                    master_name;
      block_signing_authority authority;

      template<typename Op>
      static void for_each_key( const block_signing_authority& authority, Op&& op ) {
         authority.visit([&op](const auto &a){
            a.for_each_key(std::forward<Op>(op));
         });
      }

      template<typename Op>
      void for_each_key( Op&& op ) const {
         for_each_key(authority, std::forward<Op>(op));
      }

      static std::pair<bool, size_t> keys_satisfy_and_relevant( const std::set<public_key_type>& keys, const block_signing_authority& authority ) {
         return authority.visit([&keys](const auto &a){
            return a.keys_satisfy_and_relevant(keys);
         });
      }

      std::pair<bool, size_t> keys_satisfy_and_relevant( const std::set<public_key_type>& presented_keys ) const {
         return keys_satisfy_and_relevant(presented_keys, authority);
      }

      auto to_shared(chainbase::allocator<char> alloc) const {
         auto shared_auth = authority.visit([&alloc](const auto& a) {
            return a.to_shared(alloc);
         });

         return shared_master_authority(master_name, std::move(shared_auth));
      }

      static auto from_shared( const shared_master_authority& src ) {
         master_authority result;
         result.master_name = src.master_name;
         result.authority = src.authority.visit(overloaded {
            [](const shared_block_signing_authority_v0& a) {
               return block_signing_authority_v0::from_shared(a);
            }
         });

         return result;
      }

      /**
       * ABI's for contracts expect variants to be serialized as a 2 entry array of
       * [type-name, value].
       *
       * This is incompatible with standard FC rules for
       * static_variants which produce
       *
       * [ordinal, value]
       *
       * this method produces an appropriate variant for contracts where the authority field
       * is correctly formatted
       */
      fc::variant get_abi_variant() const;

      friend bool operator == ( const master_authority& lhs, const master_authority& rhs ) {
         return tie( lhs.master_name, lhs.authority ) == tie( rhs.master_name, rhs.authority );
      }
      friend bool operator != ( const master_authority& lhs, const master_authority& rhs ) {
         return tie( lhs.master_name, lhs.authority ) != tie( rhs.master_name, rhs.authority );
      }
   };

   struct master_authority_schedule {
      master_authority_schedule() = default;

      /**
       * Up-convert a legacy master schedule
       */
      explicit master_authority_schedule( const legacy::master_schedule_type& old )
      :version(old.version)
      {
         masters.reserve( old.masters.size() );
         for( const auto& p : old.masters )
            masters.emplace_back(master_authority{ p.master_name, block_signing_authority_v0{ 1, {{p.block_signing_key, 1}} } });
      }

      master_authority_schedule( uint32_t version,  std::initializer_list<master_authority> masters )
      :version(version)
      ,masters(masters)
      {}

      auto to_shared(chainbase::allocator<char> alloc) const {
         auto result = shared_master_authority_schedule(alloc);
         result.version = version;
         result.masters.clear();
         result.masters.reserve( masters.size() );
         for( const auto& p : masters ) {
            result.masters.emplace_back(p.to_shared(alloc));
         }
         return result;
      }

      static auto from_shared( const shared_master_authority_schedule& src ) {
         master_authority_schedule result;
         result.version = src.version;
         result.masters.reserve(src.masters.size());
         for( const auto& p : src.masters ) {
            result.masters.emplace_back(master_authority::from_shared(p));
         }

         return result;
      }

      uint32_t                                       version = 0; ///< sequentially incrementing version number
      vector<master_authority>                     masters;

      friend bool operator == ( const master_authority_schedule& a, const master_authority_schedule& b )
      {
         if( a.version != b.version ) return false;
         if ( a.masters.size() != b.masters.size() ) return false;
         for( uint32_t i = 0; i < a.masters.size(); ++i )
            if( ! (a.masters[i] == b.masters[i]) ) return false;
         return true;
      }

      friend bool operator != ( const master_authority_schedule& a, const master_authority_schedule& b )
      {
         return !(a==b);
      }
   };

   /**
    * Block Header Extension Compatibility
    */
   struct master_schedule_change_extension : master_authority_schedule {

      static constexpr uint16_t extension_id() { return 1; }
      static constexpr bool     enforce_unique() { return true; }

      master_schedule_change_extension() = default;
      master_schedule_change_extension(const master_schedule_change_extension&) = default;
      master_schedule_change_extension( master_schedule_change_extension&& ) = default;

      master_schedule_change_extension( const master_authority_schedule& sched )
      :master_authority_schedule(sched) {}
   };


   inline bool operator == ( const master_authority& pa, const shared_master_authority& pb )
   {
      if(pa.master_name != pb.master_name) return false;
      if(pa.authority.which() != pb.authority.which()) return false;

      bool authority_matches = pa.authority.visit([&pb]( const auto& lhs ){
         return pb.authority.visit( [&lhs](const auto& rhs ) {
            if (lhs.threshold != rhs.threshold) return false;
            return std::equal(lhs.keys.cbegin(), lhs.keys.cend(), rhs.keys.cbegin(), rhs.keys.cend());
         });
      });

      if (!authority_matches) return false;
      return true;
   }

} } /// inery::chain

FC_REFLECT( inery::chain::legacy::master_key, (master_name)(block_signing_key) )
FC_REFLECT( inery::chain::legacy::master_schedule_type, (version)(masters) )
FC_REFLECT( inery::chain::block_signing_authority_v0, (threshold)(keys))
FC_REFLECT( inery::chain::master_authority, (master_name)(authority) )
FC_REFLECT( inery::chain::master_authority_schedule, (version)(masters) )
FC_REFLECT_DERIVED( inery::chain::master_schedule_change_extension, (inery::chain::master_authority_schedule), )

FC_REFLECT( inery::chain::shared_block_signing_authority_v0, (threshold)(keys))
FC_REFLECT( inery::chain::shared_master_authority, (master_name)(authority) )
FC_REFLECT( inery::chain::shared_master_authority_schedule, (version)(masters) )

