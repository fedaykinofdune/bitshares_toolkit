#include <bts/blockchain/chain_interface.hpp>
#include <bts/blockchain/config.hpp>
#include <fc/io/json.hpp>
#include <algorithm>
#include <sstream>

namespace bts{ namespace blockchain {
   balance_record::balance_record( const address& owner, const asset& balance_arg, name_id_type delegate_id )
   {
      balance =  balance_arg.amount;
      condition = withdraw_condition( withdraw_with_signature( owner ), balance_arg.asset_id, delegate_id );
   }

   /** returns 0 if asset id is not condition.asset_id */
   asset     balance_record::get_balance()const
   {
      return asset( balance, condition.asset_id );
   }

   address balance_record::owner()const
   {
      if( condition.type == withdraw_signature_type )
         return condition.as<withdraw_with_signature>().owner;
      else if ( condition.type == withdraw_by_name_type )
         return condition.as<withdraw_by_name>().owner;
      return address();
   }

   share_type     chain_interface::get_delegate_registration_fee()const
   {
      return (get_fee_rate() * BTS_BLOCKCHAIN_DELEGATE_REGISTRATION_FEE)/1000;
   }
   share_type    chain_interface::get_asset_registration_fee()const
   {
      return (get_fee_rate() * BTS_BLOCKCHAIN_ASSET_REGISTRATION_FEE)/1000;
   }

   bool name_record::is_valid_name( const std::string& str )
   {
      if( str.size() == 0 ) return false;
      if( str.size() > BTS_BLOCKCHAIN_MAX_NAME_SIZE ) return false;
      if( str[0] < 'a' || str[0] > 'z' ) return false;
      for( auto c : str )
      {
          if( c >= 'a' && c <= 'z' ) continue;
          else if( c >= '0' && c <= '9' ) continue;
          else if( c == '-' ) continue;
          else return false;
      }
      return true;
   }
   bool name_record::is_valid_json( const std::string& str )
   {
      return fc::json::is_valid( str );
   }

   asset_id_type chain_interface::last_asset_id()const
   {
       return get_property( chain_property_enum::last_asset_id ).as<asset_id_type>();
   }

   asset_id_type  chain_interface::new_asset_id()
   {
      auto next_id = last_asset_id() + 1;
      set_property( chain_property_enum::last_asset_id, next_id );
      return next_id;
   }

   name_id_type   chain_interface::last_name_id()const
   {
       return get_property( chain_property_enum::last_name_id ).as<name_id_type>();
   }

   name_id_type   chain_interface::new_name_id()
   {
      auto next_id = last_name_id() + 1;
      set_property( chain_property_enum::last_name_id, next_id );
      return next_id;
   }

   proposal_id_type   chain_interface::last_proposal_id()const
   {
       return get_property( chain_property_enum::last_proposal_id ).as<proposal_id_type>();
   }

   proposal_id_type   chain_interface::new_proposal_id()
   {
      auto next_id = last_proposal_id() + 1;
      set_property( chain_property_enum::last_proposal_id, next_id );
      return next_id;
   }

   std::vector<name_id_type> chain_interface::get_active_delegates()const
   { try {
      return get_property( active_delegate_list_id ).as<std::vector<name_id_type> >();
   } FC_RETHROW_EXCEPTIONS( warn, "" ) }

   void                      chain_interface::set_active_delegates( const std::vector<name_id_type>& delegate_ids )
   {
      set_property( active_delegate_list_id, fc::variant(delegate_ids) );
   }

   bool                      chain_interface::is_active_delegate( name_id_type delegate_id ) const
   { try {
      auto active = get_active_delegates();
      return active.end() != std::find( active.begin(), active.end(), delegate_id );
   } FC_RETHROW_EXCEPTIONS( warn, "", ("delegate_id",delegate_id) ) }

} }  // bts::blockchain

