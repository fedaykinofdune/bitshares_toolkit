#pragma once

#include <bts/blockchain/chain_database.hpp>
#include <bts/blockchain/config.hpp>
#include <bts/wallet/pretty.hpp>
#include <bts/wallet/wallet_db.hpp>

namespace bts { namespace wallet {
   using namespace bts::blockchain;

   namespace detail { class wallet_impl; }

   /** takes 4 parameters, current block, last block, current trx, last trx */
   typedef std::function<void(uint32_t,uint32_t,uint32_t,uint32_t)> scan_progress_callback;

   struct delegate_trust_status
   {
      fc::optional<int32_t> user_trust_level;
     //cached_master_key; //not yet implemented, eventually this can be a security check
   };

   /**
    * When transferring a balance from one individual to another it must be
    * divided up into many smaller transactions to maximize privacy.  A group
    * of smaller transactions is considered an invoice.
    *
    * These transactions should be broadcast over time rather than all at once
    * in order to maximize user privacy.
    */
   struct invoice_summary
   {
      std::map<transaction_id_type, signed_transaction> payments;
      std::string                                       from_account;
      std::string                                       to_account;
      invoice_index_type                                sending_invoice_index;
      payment_index_type                                last_sending_payment_index;
   };


   class wallet
   {
      public:
         wallet( const chain_database_ptr& chain );
         virtual ~wallet();

         /**
          *  Wallet File Management
          */
         ///@{
         void           set_data_directory( const fc::path& data_dir );
         fc::path       get_data_directory()const;

         void           create( const std::string& wallet_name, const std::string& password );
         void           open( const std::string& wallet_name, const std::string& password, fc::microseconds unlock_time = fc::microseconds() );
         void           open_file( const fc::path& wallet_filename, const std::string& password, fc::microseconds unlock_time = fc::microseconds() );
         void           close();

         void           backup_wallet( const fc::path& file );
         void           export_to_json( const fc::path& path ) const;
         void           create_from_json( const fc::path& path, const std::string& name, const std::string& passphrase );

         bool           is_open()const;
         std::string    get_name()const;
         fc::path       get_filename()const;
         ///@}

         /**
          *  Lock management & security
          */
         ///@{
         void           unlock( const fc::microseconds& timeout, const std::string& password );
         void           lock();
         void           change_passphrase(const std::string& new_passphrase);

         bool           is_unlocked()const;
         bool           is_locked()const;
         fc::time_point unlocked_until()const;
         ///@}

         /**
          *  Blockchain Scanning API
          *
          *  Looks throught the blockchain for assets, names, and other information
          *  that is relevant to this wallet.
          */
         ///@{
         /** scans the records (not the transactions) looking for assets, accounts, and names
          * registered to any private key controlled by this wallet, this is faster than
          * scanning for transactions.
          */
         void               scan_state();
         void               scan_balances();
         void               scan_names();
         void               scan_assets();

         void               scan_block( const full_block& );
         void               scan_chain( uint32_t block_num,
                                        scan_progress_callback cb = scan_progress_callback() );
         ///@}


         /**
          *  The following API calls are used to manage TITAN transactions and are the
          *  preferred API for future wallets. 
          */
         ///@{ TITAN 
         owallet_identity                lookup_identity( const std::string& name );

         void                            rename_unlisted_identity( const std::string& old_name, 
                                                                   const std::string& new_name );

         void                            add_unlisted_identity( const std::string& unregistered_name,
                                                                const public_key_type& key );

         public_key_type                 create_unlisted_identity( const std::string& identity_name );
                                         
         signed_transaction              register_identity( const std::string& unregistered_name, 
                                                            bool as_delegate = false,
                                                            const fc::variant& data = fc::variant() );
                                         
         std::vector<wallet_identity>    list_identities()const;
                                         
         void                            scan_with_identities( uint32_t start_block = 0, 
                                                               uint32_t count = -1 )const;

         uint32_t                        get_last_identity_scanned_block_number()const;

        // void                            set_default_identity( const std::string& identity_name );
        // std::string                     get_default_identity()const;

         std::vector<signed_transaction> transfer_to_identity( const std::string& to_identity_name, 
                                                               share_type num_shares, 
                                                               const std::string& asset_symbol,
                                                               const std::string& from_identity_name,
                                                               const std::string& memo_message 
                                                             );

         signed_transaction              issue_to_identity( const std::string& to_identity_name,
                                                            share_type num_shares,
                                                            const std::string& asset_symbol );
         ///@} TITAN 
         


         /**
          *  Block Generation API
          */
         ///@{
         uint32_t           get_last_scanned_block_number()const;

         /**
          *  If this wallet has any delegate keys, this method will return the time
          *  at which this wallet may produce a block.
          */
         fc::time_point_sec next_block_production_time()const;

         /** sign a block if this wallet controls the key for the active delegate, or throw */
         void               sign_block( signed_block_header& header )const;

         ///@} Block Generation API


         /**
          *  Account management API
          */
         ///@{
         wallet_account_record    create_receive_account( const std::string& account_name );
         void                     create_sending_account( const std::string& account_name, const extended_public_key& );
         wallet_account_record    get_account( const std::string& account_name )const;
         void                     rename_account( const std::string& current_account_name,
                                                  const std::string& new_account_name );

         std::map<std::string,extended_address> list_receive_accounts( uint32_t start = 0, uint32_t count = -1 )const;
         std::map<std::string,extended_address> list_sending_accounts( uint32_t start = 0, uint32_t count = -1 )const;


         void import_bitcoin_wallet( const fc::path& wallet_dat,
                                     const std::string& wallet_dat_passphrase,
                                     const std::string& account_name = "*",
                                     const std::string& invoice_memo = "" );

         void import_private_key( const private_key_type& key,
                                  const std::string& account_name = "*",
                                  const std::string& invoice_memo = "" );

         void import_wif_private_key( const std::string& wif_key,
                                      const std::string& account_name = "*",
                                      const std::string& invoice_memo = "" );
         ///@}

         /**
          *  Transaction Generation Methods
          */
         ///@{
         invoice_summary          transfer( const std::string& to_account_name,
                                            const asset& amount,
                                            const std::string& from_account_name = "*",
                                            const std::string& invoice_memo = "",
                                            const bool sign = true );

         signed_transaction       create_asset( const std::string& symbol,
                                                const std::string& asset_name,
                                                const std::string& description,
                                                const fc::variant& data,
                                                const std::string& issuer_name,
                                                share_type max_share_supply = BTS_BLOCKCHAIN_MAX_SHARES,
                                                const bool sign = true );

         signed_transaction       issue_asset( share_type amount, 
                                               const std::string& symbol,                                               
                                               const std::string& to_account_name,
                                               const bool sign = true );



         /**
          * if the active_key is null then the active key will be made the same as the master key.
          * if the name already exists then it will be updated if this wallet controls the active key
          * or master key
          */
         signed_transaction reserve_name( const std::string& name,
                                          const fc::variant& json_data,
                                          bool as_delegate = false,
                                          const bool sign = true );

         signed_transaction update_name( const std::string& name,
                                         fc::optional<fc::variant> json_data,
                                         fc::optional<public_key_type> active = fc::optional<public_key_type>(),
                                         bool as_delegate = false,
                                         const bool sign = true );

         signed_transaction submit_proposal( const std::string& name,
                                             const std::string& subject,
                                             const std::string& body,
                                             const std::string& proposal_type,
                                             const fc::variant& data,
                                             const bool sign = true );

         signed_transaction vote_proposal( const std::string& name, 
                                           proposal_id_type proposal_id, 
                                           uint8_t vote,
                                           const bool sign = true);


         ///@} Transaction Generation Methods
 
         pretty_transaction                      to_pretty_trx( wallet_transaction_record trx_rec,
                                                                int number = 0 );



         std::string  get_symbol( asset_id_type asset_id )const;

         void                                         set_delegate_trust_status(const std::string& delegate_name, 
                                                                                fc::optional<int32_t> trust_level);
         delegate_trust_status                        get_delegate_trust_status(const std::string& delegate_name) const;
         std::map<std::string, delegate_trust_status> list_delegate_trust_status() const;

         bool                                       is_sending_address( const address& addr )const;
         bool                                       is_receive_address( const address& addr )const;



         /**
          *  Bitcoin compatiblity
          */
         ///@{
         address                                    get_new_address( const std::string& account_name = "", uint32_t invoice = 0 );
         public_key_type                            get_new_public_key( const std::string& account_name = "", uint32_t invoice = 0 );
         extended_public_key                        get_new_extended_public_key( const std::string& account_name = "" );

         std::unordered_map<address,std::string>    get_receive_addresses()const;
         std::unordered_map<address,std::string>    get_send_addresses()const;

         asset                                      get_balance( const std::string& symbol = BTS_ADDRESS_PREFIX,
                                                                 const std::string& account_name = "*" );

         std::vector<asset>                         get_all_balances( const std::string& account_name = "*" );
         ///@}

         std::vector<wallet_transaction_record>     get_transaction_history( unsigned count = 0 )const;

         fc::optional<address>                      get_owning_address( const balance_id_type& id )const;
         fc::optional<wallet_account_record>        get_account_record( const address& addr)const;

         std::unordered_map<transaction_id_type,wallet_transaction_record>  transactions( const std::string& account_name = "*" )const;
         std::unordered_map<name_id_type,       wallet_name_record>         names( const std::string& account_name = "*" )const;
         std::unordered_map<balance_id_type,    wallet_balance_record>      balances( const std::string& account_name = "*" )const;
         std::unordered_map<asset_id_type,      wallet_asset_record>        assets( const std::string& account_name = "*" )const;

         /** signs transaction with the specified keys for the specified addresses */
         void  sign_transaction( signed_transaction& trx, const std::unordered_set<address>& req_sigs );

      private:
         std::unique_ptr<detail::wallet_impl> my;
   };

   typedef std::shared_ptr<wallet> wallet_ptr;

} } // bts::wallet

FC_REFLECT( bts::wallet::invoice_summary, (payments)(from_account)(to_account)(sending_invoice_index)(last_sending_payment_index) )
FC_REFLECT( bts::wallet::delegate_trust_status, (user_trust_level) )
