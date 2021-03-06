#define BOOST_TEST_MODULE BlockchainTests2
#include <boost/test/unit_test.hpp>
#include <bts/blockchain/chain_database.hpp>
#include <bts/wallet/wallet.hpp>
#include <bts/blockchain/config.hpp>
#include <bts/blockchain/time.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>
#include <fc/io/json.hpp>
#include <fc/thread/thread.hpp>
#include <iostream>

using namespace bts::blockchain;
using namespace bts::wallet;

const char* test_keys = R"([
  "dce167e01dfd6904015a8106e0e1470110ef2d5b0b18ba7a83cb8204e25c6b5f",
  "7fee1dc3110ba4abe134822c257c9db5beadbe557763cc54e3dc59b699978a50",
  "4e42e82970d3307d26634572cddbf8424c10cee1c8c7fcebdd9417942a08a1bd",
  "e44baa4f693f1bd71ba8f6b8509c56dd41206e2cff07efcf7b42fc79464a1c59",
  "18fd5207b1a0d72465b8f3ed06b44232a366a76f559c5da3aec9f306e179278f",
  "6dc61837b51d0bb80143c1a7ba5c957f122faa96e0a6ff76ba25f9ff42ef69fd",
  "45b0a66b5016618700b4d4fbfa85efd1b0724e8ae95b09a6c9ec850a05254f48",
  "602751d179b75da5432c64a3360a7309609636056c31deae37b8441230c968eb",
  "8ced7ac956e1755e87c21b1b265979c848c79b3bea3b855bb5b819d3baa72ed2",
  "90ef5e50773c90368597e46eaf1b563f76f879aa8969c2e7a2198847f93324c4"
])";

BOOST_AUTO_TEST_CASE( titan )
{ try {
   fc::ecc::private_key to_private_key   = fc::ecc::private_key::generate();
   fc::ecc::private_key from_private_key = fc::ecc::private_key::generate();

   withdraw_by_name cond;
   ilog( "encrypting it.." );
   cond.encrypt_memo_data( fc::ecc::private_key::generate(),
                           to_private_key.get_public_key(),
                           from_private_key,
                           "01234567890123456789" );
   ilog( "now decrypt it..." );
   auto result = cond.decrypt_memo_data( to_private_key );
   FC_ASSERT( result.valid() );
   FC_ASSERT( result->has_valid_signature );
   FC_ASSERT( result->get_message() == "01234567890123456789", "",("get_message",result->get_message() ) );

  }
  catch ( const fc::exception& e )
  {
     elog( "${e}", ("e",e.to_detail_string() ) );
     throw;
  }
}

BOOST_AUTO_TEST_CASE( price_math )
{
   try {
      try {
      std::string test_price ="3.14 5/6";
      price p( test_price  );
      ilog( "price: ${p}", ("p",p) );
      std::cout << test_price << "  ==?  " << std::string(p) <<"\n";


      price priceA( 0.000234, 6, 5 );
      std::string string_from_A( priceA );
      price priceA_from_string( string_from_A );
      
      auto tmp = asset( 1, 2 ) / asset( 3, 1 );
      ilog( "${tmp}", ("tmp",tmp) );

      ilog( "${price_a}", ("price_a",priceA) );
      ilog( "${price_a}", ("price_a",std::string(priceA)) );
      ilog( "${price_a}", ("price_a",std::string(priceA_from_string)) );

      FC_ASSERT( priceA == priceA_from_string, "",
                 ("priceA",priceA)("priceA_from_string",priceA_from_string)("string_from_A",string_from_A) );


      } FC_RETHROW_EXCEPTIONS( warn, "" )

   } catch ( const fc::exception& e )
   {
      elog( "${e}", ("e",e.to_detail_string() ) );
      throw;
   }
}

BOOST_AUTO_TEST_CASE( block_signing )
{
   try {
      fc::ecc::private_key signer = fc::ecc::private_key::generate();
      auto pub_key = signer.get_public_key();
      ilog( "pub_key: ${pub}", ("pub",pub_key) );
      full_block blk;

      signed_block_header& header = blk;
      header.sign( signer );
      FC_ASSERT( blk.validate_signee( pub_key ) );
   }
   catch ( const fc::exception& e )
   {
      elog( "${e}", ("e",e.to_detail_string() ) );
      throw;
   }
}


BOOST_AUTO_TEST_CASE( wallet_test )
{
      fc::temp_directory dir;

      chain_database_ptr blockchain = std::make_shared<chain_database>();
      blockchain->open( dir.path(), "genesis.dat" );

      wallet  my_wallet( blockchain );
      my_wallet.set_data_directory( dir.path() );
      my_wallet.create(  "my_wallet", "password" );

      my_wallet.close();
      my_wallet.open( "my_wallet", "password" );
      my_wallet.unlock( fc::seconds( 10000000 ), "password" );
      my_wallet.import_private_key( fc::variant("dce167e01dfd6904015a8106e0e1470110ef2d5b0b18ba7a83cb8204e25c6b5f").as<fc::ecc::private_key>() );
      my_wallet.close();
      my_wallet.open( "my_wallet", "password" );
}


BOOST_AUTO_TEST_CASE( negative_transfer_test )
{
    fc::temp_directory client1Dir;
    fc::temp_directory client2Dir;

    chain_database_ptr client1Chain = std::make_shared<chain_database>();
    chain_database_ptr client2Chain = std::make_shared<chain_database>();
    client1Chain->open(client1Dir.path(), "genesis.dat");
    client2Chain->open(client2Dir.path(), "genesis.dat");

    wallet client1( client1Chain );
    client1.set_data_directory( client1Dir.path() );
    client1.create( "client1", "client1Pass" );
    client1.unlock( fc::seconds( 10000000 ), "client1Pass" );
    client1.import_private_key( fc::variant("dce167e01dfd6904015a8106e0e1470110ef2d5b0b18ba7a83cb8204e25c6b5f").as<fc::ecc::private_key>() );
    client1.scan_state();
    client1.close();
    client1.open( "client1", "client1Pass" );
    client1.unlock( fc::seconds( 10000000 ), "client1Pass" );

    ilog("Balance: ${bal}", ("bal",client1.get_balance()));

    wallet client2( client2Chain );
    client2.set_data_directory( client2Dir.path() );
    client2.create( "client2", "client2Pass" );
    client2.unlock( fc::seconds( 20000000 ), "client2Pass" );

    auto recieveAddress = client2.create_receive_account( "client2Receive" );
    client1.create_sending_account( "sendAddress", recieveAddress.extended_key );

    bool failed = false;
    try
    {
        auto invoice_sum = client1.transfer( "sendAddress", asset(-500) );

        for( auto trx : invoice_sum.payments )
        {
           client1Chain->store_pending_transaction( trx.second );
           client2Chain->store_pending_transaction( trx.second );
           ilog( "trx: ${trx}", ("trx",trx.second) );
        }
    }
    catch ( const fc::exception& e)
    {
        failed = true;
    }

    BOOST_REQUIRE( failed );
}


BOOST_AUTO_TEST_CASE( hundred_block_transfer_test )
{
   try {
      fc::temp_directory dir2;
      fc::temp_directory dir;

      chain_database_ptr blockchain = std::make_shared<chain_database>();
      blockchain->open( dir.path(), "genesis.dat" );

      chain_database_ptr blockchain2 = std::make_shared<chain_database>();
      blockchain2->open( dir2.path(), "genesis.dat" );

      elog( "last asset id: ${id}", ("id",blockchain->last_asset_id() ) );
      elog( "last name id: ${id}", ("id",blockchain->last_name_id() ) );

      ilog( "." );
      auto delegate_list = blockchain->get_delegates_by_vote();
      ilog( "delegates: ${delegate_list}", ("delegate_list",delegate_list) );
      for( uint32_t i = 0; i < delegate_list.size(); ++i )
      {
         auto name_rec = blockchain->get_name_record( delegate_list[i] );
         ilog( "${i}] ${delegate}", ("i",i) ("delegate", name_rec ) );
      }
      blockchain->scan_names( [=]( const name_record& a ) {
         ilog( "\nname: ${a}", ("a",fc::json::to_pretty_string(a) ) );
      });

      blockchain->scan_assets( [=]( const asset_record& a ) {
         ilog( "\nasset: ${a}", ("a",fc::json::to_pretty_string(a) ) );
      });


      wallet  my_wallet( blockchain );
      my_wallet.set_data_directory( dir.path() );
      my_wallet.create(  "my_wallet", "password" );
      my_wallet.unlock( fc::seconds( 10000000 ), "password" );

      wallet  your_wallet( blockchain2 );
      your_wallet.set_data_directory( dir2.path() );
      your_wallet.create(  "your_wallet", "password" );
      your_wallet.unlock( fc::seconds( 10000000 ), "password" );

      auto keys = fc::json::from_string( test_keys ).as<std::vector<fc::ecc::private_key> >();
      for( auto key: keys )
      {
         my_wallet.import_private_key( key );
      }
      my_wallet.scan_state();
      my_wallet.close();
      my_wallet.open( "my_wallet", "password" );
      my_wallet.unlock( fc::seconds( 10000000 ), "password" );

      auto result = blockchain->get_names( "", 1000 );

   //   my_wallet.scan_state();

         ilog( "my balance: ${my}   your balance: ${your}",
               ("my",my_wallet.get_balance())
               ("your",your_wallet.get_balance()) );

      ilog( "." );
      share_type total_sent = 0;

      std::string your_account_name = "my-send-to-your";
      auto your_account = your_wallet.create_receive_account( your_account_name ).extended_key;
      my_wallet.create_sending_account( your_account_name, your_account );
      for( uint32_t i = 10; i < 100; ++i )
      {
         auto next_block_time = my_wallet.next_block_production_time();
         ilog( "next block production time: ${t}", ("t",next_block_time) );
         FC_ASSERT( next_block_time != blockchain->now() );

         auto wait_until_time = my_wallet.next_block_production_time();
         auto sleep_time = wait_until_time - bts::blockchain::now(); //fc::time_point::now();
         ilog( "waiting: ${t}s", ("t",sleep_time.count()/1000000) );
         bts::blockchain::advance_time((uint32_t)(sleep_time.count() / 1000000));
         //fc::usleep( sleep_time );


         full_block next_block = blockchain->generate_block( next_block_time );
         my_wallet.sign_block( next_block );
         ilog( "                MY_WALLET   PUSH_BLOCK" );
         blockchain->push_block( next_block );
         ilog( "                YOUR_WALLET   PUSH_BLOCK" );
         blockchain2->push_block( next_block );


         ilog( "BEFORE: my balance: ${my}   your balance: ${your}",
               ("my",my_wallet.get_balance())
               ("your",your_wallet.get_balance()) );
         /** 
          * close and reopen the wallet to make sure we do not lose state.
          */
         //my_wallet.close();
         your_wallet.is_receive_address( address() );
         your_wallet.close();
         //my_wallet.open( "my_wallet", "password" );
         //my_wallet.unlock( fc::microseconds::maximum(), "password" );
         your_wallet.open( "your_wallet", "password" );
         your_wallet.unlock( fc::microseconds::maximum(), "password" );
         your_wallet.is_receive_address( address() );
         ilog( "AFTER: my balance: ${my}   your balance: ${your}",
               ("my",my_wallet.get_balance())
               ("your",your_wallet.get_balance()) );

         FC_ASSERT( total_sent == your_wallet.get_balance().amount, "",
                    ("toatl_sent",total_sent)("balance",your_wallet.get_balance().amount));

         bts::blockchain::advance_time(1);
         //fc::usleep( fc::microseconds(1200000) );

         for( uint64_t t = 1; t <= 2; ++t )
         {
            auto amnt = rand()%30000 + 1;
            auto invoice_sum = my_wallet.transfer( your_account_name, asset( amnt ) );
            for( auto trx : invoice_sum.payments )
            {
               blockchain->store_pending_transaction( trx.second );
               blockchain2->store_pending_transaction( trx.second );
               //ilog( "trx: ${trx}", ("trx",trx.second) );
            }
            total_sent += amnt;
         }

      }

      blockchain->close();
   }
   catch ( const fc::exception& e )
   {
      elog( "${e}", ("e",e.to_detail_string() ) );
      throw;
   }
   catch ( ... )
   {
      elog( "exception" );
      throw;
   }
}

BOOST_AUTO_TEST_CASE( name_registration_test )
{
    try {
        fc::temp_directory dir;

        chain_database_ptr blockchain = std::make_shared<chain_database>();
        blockchain->open( dir.path(), "genesis.dat" );

        elog( "last name id: ${id}", ("id", blockchain->last_name_id()) );

        ilog(".");
        auto delegate_list = blockchain->get_delegates_by_vote();
        ilog( "delegates: ${delegate_list}", ("delegate_list", delegate_list) );
        for (uint32_t i = 0; i < delegate_list.size(); ++i)
        {
            auto name_rec = blockchain->get_name_record( delegate_list[i] );
            ilog( "${i}] ${delegate}", ("i", i) ("delegate", name_rec) );
        }
        blockchain->scan_names([=]( const name_record& a ) {
  //          ilog( "\nname: ${a}", ("a", fc::json::to_pretty_string(a)) );
        });

        blockchain->scan_assets([=]( const asset_record& a ) {
  //          ilog( "\nasset: ${a}", ("a", fc::json::to_pretty_string(a)) );
        });


        wallet  my_wallet( blockchain );
        my_wallet.set_data_directory( dir.path() );
        my_wallet.create( "my_wallet", "password" );
        my_wallet.unlock( fc::seconds(10000000), "password" );

        auto keys = fc::json::from_string(test_keys).as<std::vector<fc::ecc::private_key> >();
        for (auto key : keys)
        {
            my_wallet.import_private_key( key );
        }
        my_wallet.scan_state();
        my_wallet.close();
        my_wallet.open( "my_wallet", "password" );
        my_wallet.unlock( fc::seconds(10000000), "password" );

        ilog( "my balance: ${my}", ("my", my_wallet.get_balance()) );

        ilog(".");
        //share_type total_sent = 0;

        std::string name_prefix = "test-name";
        for ( uint32_t i = 0; i < 10; ++i )
        {
            // reserve names or register delegates depending on i.
            bool is_delegate = (i % 2 == 0);
            auto trx = my_wallet.reserve_name( name_prefix + fc::to_string(i), fc::to_string(i), is_delegate );
            ilog( "trx: ${trx}", ("trx", trx) );
            blockchain->store_pending_transaction( trx );

            auto next_block_time = my_wallet.next_block_production_time();
            ilog( "next block production time: ${t}", ("t", next_block_time) );

            auto wait_until_time = my_wallet.next_block_production_time();
            auto sleep_time = wait_until_time - bts::blockchain::now(); //fc::time_point::now();
            ilog( "waiting: ${t}s", ("t", sleep_time.count() / 1000000) );
            bts::blockchain::advance_time( sleep_time.count() / 1000000 );

            full_block next_block = blockchain->generate_block( next_block_time );
            my_wallet.sign_block( next_block );
            ilog("                MY_WALLET   PUSH_BLOCK ${i}", ("i", i));
            blockchain->push_block( next_block );

            bts::blockchain::advance_time( 1 );

            auto name_record = blockchain->get_name_record( name_prefix + fc::to_string(i) );
            FC_ASSERT( !!name_record
                && name_record->json_data.as_string() == fc::to_string(i)
                && name_record->is_delegate() == is_delegate
                , "", ("name_record", *name_record)("name", name_prefix + fc::to_string(i))("json_data", fc::to_string(i)));
        }
        
        bool failed = false;
        
        try
        {
            // this name has already been registered, so exceptions should be throw
            my_wallet.reserve_name(name_prefix + fc::to_string(1), fc::to_string(1));
        }
        catch ( const fc::exception& e)
        {
            failed = true;
        }
        
        BOOST_REQUIRE(failed);

        // test updating name records and change all test none-delegate names to delegate
        std::string json_prefix = "url: test";
        for ( uint32_t i = 0; i < 10; ++i )
        {
            auto trx = my_wallet.update_name( name_prefix + fc::to_string(i),
                fc::variant(json_prefix + fc::to_string(i)), fc::optional<public_key_type>(), 
                /*change to delegate*/true);
            blockchain->store_pending_transaction( trx );
        }

        {
            auto next_block_time = my_wallet.next_block_production_time();
            ilog( "next block production time: ${t}", ("t", next_block_time) );

            auto wait_until_time = my_wallet.next_block_production_time();
            auto sleep_time = wait_until_time - bts::blockchain::now(); //fc::time_point::now();
            ilog( "waiting: ${t}s", ("t", sleep_time.count() / 1000000) );
            bts::blockchain::advance_time( sleep_time.count() / 1000000 );

            full_block next_block = blockchain->generate_block( next_block_time );
            my_wallet.sign_block( next_block );
            ilog("                MY_WALLET   PUSH_BLOCK");
            blockchain->push_block( next_block );

            bts::blockchain::advance_time(1);
        }

        auto result = blockchain->get_names( name_prefix, 10 );

        for (uint32_t i = 0; i < 10; ++i)
        {
            FC_ASSERT( result[i].name == name_prefix + fc::to_string(i), "", ("name", result[i].name) );
            FC_ASSERT( result[i].json_data.as_string()
                == (json_prefix + fc::to_string(i)),
                "", ("json", result[i].json_data) );

            // assert all are delegates
            FC_ASSERT( result[i].is_delegate(), "", ("i", i)("delegate_info", !!(result[i].delegate_info)) );
        }

        blockchain->close();
    }
    catch (const fc::exception& e)
    {
        elog("${e}", ("e", e.to_detail_string()));
        throw;
    }
    catch (...)
    {
        elog("exception");
        throw;
    }
}

BOOST_AUTO_TEST_CASE( asset_record_test )
{
    try {
        fc::temp_directory dir;

        chain_database_ptr blockchain = std::make_shared<chain_database>();
        blockchain->open( dir.path(), "genesis.dat" );

        elog( "last name id: ${id}", ("id", blockchain->last_name_id()) );

        ilog(".");
        auto delegate_list = blockchain->get_delegates_by_vote();
        ilog( "delegates: ${delegate_list}", ("delegate_list", delegate_list) );
        for (uint32_t i = 0; i < delegate_list.size(); ++i)
        {
            auto name_rec = blockchain->get_name_record( delegate_list[i] );
            ilog( "${i}] ${delegate}", ("i", i) ("delegate", name_rec) );
        }
        blockchain->scan_names([=]( const name_record& a ) {
  //          ilog( "\nname: ${a}", ("a", fc::json::to_pretty_string(a)) );
        });

        blockchain->scan_assets([=]( const asset_record& a ) {
  //          ilog( "\nasset: ${a}", ("a", fc::json::to_pretty_string(a)) );
        });


        wallet  my_wallet( blockchain );
        my_wallet.set_data_directory( dir.path() );
        my_wallet.create( "my_wallet", "password" );
        my_wallet.unlock( fc::seconds(10000000), "password" );

        auto keys = fc::json::from_string(test_keys).as<std::vector<fc::ecc::private_key> >();
        for (auto key : keys)
        {
            my_wallet.import_private_key( key );
        }
        my_wallet.scan_state();
        my_wallet.close();
        my_wallet.open( "my_wallet", "password" );
        my_wallet.unlock( fc::seconds(10000000), "password" );

        ilog( "my balance: ${my}", ("my", my_wallet.get_balance()) );

        ilog(".");
        //share_type total_sent = 0;

        std::string name_prefix = "test-name";
        uint64_t total_supply = 100000000;
        uint64_t current_share_supply = 5000000;
        for ( uint32_t i = 0; i < 5; ++i )
        {
            bool failed = false;

            try
            {
                auto asset_trx = my_wallet.create_asset("ABC", "Test_Asset" + fc::to_string(i), "", "", name_prefix + fc::to_string(i), total_supply);
                blockchain->store_pending_transaction( asset_trx );
            }
            catch (const fc::exception&)
            {
                failed = true;
            }

            BOOST_REQUIRE(failed);  // issue name is not registered yet.


            // reserve names or register delegates depending on i.
            auto trx = my_wallet.reserve_name( name_prefix + fc::to_string(i), fc::to_string(i), false );
            ilog( "trx: ${trx}", ("trx", trx) );
            blockchain->store_pending_transaction( trx );

            if ( i > 0) // using previous i-1 name to create assets
            {
                // issue name should already been registered, than create the asset
                auto asset_trx = my_wallet.create_asset("AB" + fc::to_string(i-1), "Test_Asset" + fc::to_string(i-1), "", "", name_prefix + fc::to_string(i-1), total_supply);
                blockchain->store_pending_transaction( asset_trx );
            }

            if ( i > 1 ) // test updating i - 2 assets, and issue the assets.
            {
                auto asset_record = blockchain->get_asset_record("AB" + fc::to_string(i-2));
                FC_ASSERT(!! asset_record, "asset should already be created");
                FC_ASSERT( asset_record->available_shares() == total_supply, "no asset should already been issued yet" );
                FC_ASSERT( ! asset_record->is_market_issued(), "asset is issued by name, not by market" );

                failed = false;
                try
                {
                    auto asset_trx = my_wallet.create_asset("AB" + fc::to_string(i-2), "Test_Asset" + fc::to_string(i-2), "", "", name_prefix + fc::to_string(i-2), total_supply);
                    blockchain->store_pending_transaction( asset_trx );
                }
                catch (const fc::exception&)
                {
                    failed = true;
                }
                BOOST_REQUIRE(failed);  // asset should already created.

                
                auto asset_trx = my_wallet.issue_asset(current_share_supply, "AB" + fc::to_string(i-2), "*");
                blockchain->store_pending_transaction( asset_trx );
            }

            if ( i > 2 )
            {
                auto asset_record = blockchain->get_asset_record("AB" + fc::to_string(i-3));
                FC_ASSERT(!! asset_record);
                FC_ASSERT(asset_record->current_share_supply >= current_share_supply, "current share supply should have already be issued", ("asset_record", asset_record));
                // TODO: how issued asset become balance records?
                // FC_ASSERT(my_wallet.get_balance("AB" + fc::to_string(i-3)) > 0, "5000000 should have already be issued to me");
            }

            // TODO: test update asset operation

            auto next_block_time = my_wallet.next_block_production_time();
            ilog( "next block production time: ${t}", ("t", next_block_time) );

            auto wait_until_time = my_wallet.next_block_production_time();
            auto sleep_time = wait_until_time - bts::blockchain::now(); //fc::time_point::now();
            ilog( "waiting: ${t}s", ("t", sleep_time.count() / 1000000) );
            bts::blockchain::advance_time( sleep_time.count() / 1000000 );

            full_block next_block = blockchain->generate_block( next_block_time );
            my_wallet.sign_block( next_block );
            ilog("                MY_WALLET   PUSH_BLOCK ${i}", ("i", i));
            blockchain->push_block( next_block );

            bts::blockchain::advance_time( 1 );
            my_wallet.scan_state();
        }

        blockchain->close();
    }
    catch (const fc::exception& e)
    {
        elog("${e}", ("e", e.to_detail_string()));
        throw;
    }
    catch (...)
    {
        elog("exception");
        throw;
    }
}

BOOST_AUTO_TEST_CASE( basic_fork_test )
{
   try {
    // the purpose of this test is to validate the fork handling
    // when two alternative forks are produced and then merged.

    fc::temp_directory my_dir;
    fc::temp_directory your_dir;

    chain_database_ptr my_chain = std::make_shared<chain_database>();
    my_chain->open( my_dir.path(), "genesis.dat" );

    chain_database_ptr your_chain = std::make_shared<chain_database>();
    your_chain->open( your_dir.path(), "genesis.dat" );

    wallet  my_wallet( my_chain );
    my_wallet.set_data_directory( my_dir.path() );
    my_wallet.create(  "my_wallet", "password" );
    my_wallet.unlock( fc::seconds( 10000000 ), "password" );

    wallet  your_wallet( your_chain );
    your_wallet.set_data_directory( your_dir.path() );
    your_wallet.create(  "your_wallet", "password" );
    your_wallet.unlock( fc::seconds( 10000000 ), "password" );


    auto keys = fc::json::from_string( test_keys ).as<std::vector<fc::ecc::private_key> >();
    for( uint32_t i = 0; i < keys.size(); ++i )
    {
       if( i % 3 == 0 )
          my_wallet.import_private_key( keys[i] );
       else
          your_wallet.import_private_key( keys[i] );
    }
    my_wallet.scan_state();
    your_wallet.scan_state();


    // produce blocks for 30 seconds...
    for( uint32_t i = 0; i < 40; ++i )
    {
       auto now = bts::blockchain::now(); //fc::time_point::now();
       std::cerr << "now: "<< std::string( fc::time_point(now) ) << "\n";
       auto my_next_block_time = my_wallet.next_block_production_time();
       if( my_next_block_time == now )
       {
          auto my_block = my_chain->generate_block( my_next_block_time );
          std::cerr << "producing my block: "<< my_block.block_num <<"\n";
          my_wallet.sign_block( my_block );
          my_chain->push_block( my_block );
          if( i < 5 ) your_chain->push_block( my_block );
       }
       auto your_next_block_time = your_wallet.next_block_production_time();
       if( your_next_block_time == now )
       {
          auto your_block = your_chain->generate_block( your_next_block_time );
          std::cerr << "producing your block: "<< your_block.block_num <<"\n";
          your_wallet.sign_block( your_block );
          your_chain->push_block( your_block );
          if( i < 5 ) my_chain->push_block( your_block );
       }
       uint32_t sleep_time_sec = (uint32_t)(BTS_BLOCKCHAIN_BLOCK_INTERVAL_SEC  - (now.sec_since_epoch() % BTS_BLOCKCHAIN_BLOCK_INTERVAL_SEC));
       std::cerr << "sleeping " << sleep_time_sec << "s \n";
       bts::blockchain::advance_time(sleep_time_sec);
       std::cerr <<"\n\n\n\n";
      // fc::usleep( fc::seconds( sleep_time_sec ) );
    }

    // we now have two chains of different lengths
    // simulate the two chains syncing up by pushing the blocks in order...

    std::cerr << "synchronizing chains\n";
    int32_t your_length = your_chain->get_head_block_num();
    std::cerr << "your length: "<<your_length << "\n";
    for( int32_t i = your_length-1; i > 0 ; --i )
    {
       try {
          auto b = your_chain->get_block( i );
          my_chain->push_block(b);
          std::cerr << "push block: " << b.block_num
                    << "    my length: " << my_chain->get_head_block_num()
                    << "  your_length: " << your_length << " \n";
       }
       catch ( const fc::exception& e )
       {
          std::cerr << "    exception: " << e.to_string() << "\n";
       }
    }
       auto b = your_chain->get_block( your_length );
       my_chain->push_block(b);
       std::cerr << "push block: " << b.block_num
                 << "    my length: " << my_chain->get_head_block_num()
                 << "  your_length: " << your_length << " \n";

    my_chain->export_fork_graph( "fork_graph.dot" );
    FC_ASSERT( my_chain->get_head_block_num() == your_chain->get_head_block_num() );
  }
  catch ( const fc::exception& e )
  {
     elog( "${e}", ("e",e.to_detail_string() ) );
     throw;
  }
}

BOOST_AUTO_TEST_CASE( basic_chain_test )
{
   try {
    // the purpose of this test is to validate the fork handling
    // when two alternative forks are produced and then merged.

    fc::temp_directory my_dir;
    fc::temp_directory your_dir;

    chain_database_ptr my_chain = std::make_shared<chain_database>();
    my_chain->open( my_dir.path(), "genesis.dat" );

    chain_database_ptr your_chain = std::make_shared<chain_database>();
    your_chain->open( your_dir.path(), "genesis.dat" );

    wallet  my_wallet( my_chain );
    my_wallet.set_data_directory( my_dir.path() );
    my_wallet.create(  "my_wallet", "password" );
    my_wallet.unlock( fc::seconds( 10000000 ), "password" );


    auto keys = fc::json::from_string( test_keys ).as<std::vector<fc::ecc::private_key> >();
    for( uint32_t i = 0; i < keys.size(); ++i )
    {
          my_wallet.import_private_key( keys[i] );
    }
    my_wallet.scan_state();


    // produce blocks for 30 seconds...
    for( uint32_t i = 0; i < 20; ++i )
    {
       auto now = bts::blockchain::now(); //fc::time_point::now();
       std::cerr << "now: "<< std::string( fc::time_point(now) ) << "\n";
       auto my_next_block_time = my_wallet.next_block_production_time();
       if( my_next_block_time == now )
       {
          auto my_block = my_chain->generate_block( my_next_block_time );
          std::cerr << "producing my block: "<< my_block.block_num <<"\n";
          my_wallet.sign_block( my_block );
          my_chain->push_block( my_block );
       }
       uint32_t sleep_time_sec = (uint32_t)(BTS_BLOCKCHAIN_BLOCK_INTERVAL_SEC  - (now.sec_since_epoch() % BTS_BLOCKCHAIN_BLOCK_INTERVAL_SEC));
       std::cerr << "sleeping " << sleep_time_sec << "s \n";
       bts::blockchain::advance_time(sleep_time_sec);
      // fc::usleep( fc::seconds( sleep_time_sec ) );
    }
  }
  catch ( const fc::exception& e )
  {
     elog( "${e}", ("e",e.to_detail_string() ) );
     throw;
  }
}

BOOST_AUTO_TEST_CASE( undo_state_test )
{
    try {
        // the purpose of this test is to test the undo state
        // when two alternative forks are produced and then merged.
        
        fc::temp_directory my_dir;
        fc::temp_directory your_dir;
        
        chain_database_ptr my_chain = std::make_shared<chain_database>();
        my_chain->open( my_dir.path(), "genesis.dat" );
        
        chain_database_ptr your_chain = std::make_shared<chain_database>();
        your_chain->open( your_dir.path(), "genesis.dat" );
        
        wallet  my_wallet( my_chain );
        my_wallet.set_data_directory( my_dir.path() );
        my_wallet.create(  "my_wallet", "password" );
        my_wallet.unlock( fc::seconds( 10000000 ), "password" );
        
        wallet  your_wallet( your_chain );
        your_wallet.set_data_directory( your_dir.path() );
        your_wallet.create(  "your_wallet", "password" );
        your_wallet.unlock( fc::seconds( 10000000 ), "password" );
        
        
        auto keys = fc::json::from_string( test_keys ).as<std::vector<fc::ecc::private_key> >();
        for( uint32_t i = 0; i < keys.size(); ++i )
        {
            if( i % 3 == 0 )
                my_wallet.import_private_key( keys[i] );
            else
                your_wallet.import_private_key( keys[i] );
        }
        my_wallet.scan_state();
        your_wallet.scan_state();
        
        
        std::string name_prefix = "test-name-";
        // produce blocks for 30 seconds...
        std::vector<std::string> your_reserved_names;
        for( uint32_t i = 0; i < 40; ++i )
        {
            auto now = bts::blockchain::now(); //fc::time_point::now();
            std::cerr << "now: "<< std::string( fc::time_point(now) ) << "\n";
            auto my_next_block_time = my_wallet.next_block_production_time();
            if( my_next_block_time == now )
            {
                auto trx = my_wallet.reserve_name( "my" + name_prefix + fc::to_string(i), fc::to_string(i), false );
                my_chain->store_pending_transaction( trx );
                
                auto my_block = my_chain->generate_block( my_next_block_time );
                std::cerr << "producing my block: "<< my_block.block_num <<"\n";
                my_wallet.sign_block( my_block );
                my_chain->push_block( my_block );
                if( i < 5 ) your_chain->push_block( my_block );
            }
            auto your_next_block_time = your_wallet.next_block_production_time();
            if( your_next_block_time == now )
            {
                auto trx = my_wallet.reserve_name( "your" + name_prefix + fc::to_string(i), fc::to_string(i), false );
                your_reserved_names.push_back("your" + name_prefix + fc::to_string(i));
                your_chain->store_pending_transaction( trx );
                
                auto your_block = your_chain->generate_block( your_next_block_time );
                std::cerr << "producing your block: "<< your_block.block_num <<"\n";
                your_wallet.sign_block( your_block );
                your_chain->push_block( your_block );
                if( i < 5 ) my_chain->push_block( your_block );
            }
            uint32_t sleep_time_sec = (uint32_t)(BTS_BLOCKCHAIN_BLOCK_INTERVAL_SEC  - (now.sec_since_epoch() % BTS_BLOCKCHAIN_BLOCK_INTERVAL_SEC));
            std::cerr << "sleeping " << sleep_time_sec << "s \n";
            bts::blockchain::advance_time(sleep_time_sec);
            std::cerr <<"\n\n\n\n";
            // fc::usleep( fc::seconds( sleep_time_sec ) );
        }
        
        bool your_chain_longer_than_mine = false;
        if ( your_chain->get_head_block_num() > my_chain->get_head_block_num() )
        {
            your_chain_longer_than_mine = true;
        }
        
        // we now have two chains of different lengths
        // simulate the two chains syncing up by pushing the blocks in order...
        
        std::cerr << "synchronizing chains\n";
        int32_t your_length = your_chain->get_head_block_num();
        std::cerr << "your length: "<<your_length << "\n";
        for( int32_t i = your_length-1; i > 0 ; --i )
        {
            try {
                auto b = your_chain->get_block( i );
                my_chain->push_block(b);
                std::cerr << "push block: " << b.block_num
                << "    my length: " << my_chain->get_head_block_num()
                << "  your_length: " << your_length << " \n";
            }
            catch ( const fc::exception& e )
            {
                std::cerr << "    exception: " << e.to_string() << "\n";
            }
        }
        
        auto b = your_chain->get_block( your_length );
        my_chain->push_block(b);
        std::cerr << "push block: " << b.block_num
        << "    my length: " << my_chain->get_head_block_num()
        << "  your_length: " << your_length << " \n";
        
        // my_chain->export_fork_graph( "fork_graph.dot" );
        // undo state for my chain should after 5 blocks
        for ( uint32_t i = 5; i < 40; i ++)
        {
            auto record = my_chain->get_name_record("my" + name_prefix + fc::to_string(i));
            FC_ASSERT(your_chain_longer_than_mine ^ (!!record), "my chain's state after 5th block should have already been undo ${r}, your chain is longer ${y}", ("r", *record)("y", your_chain_longer_than_mine));
        }
        
        for ( auto your_name : your_reserved_names )
        {
            auto record = my_chain->get_name_record(your_name);
            FC_ASSERT(your_chain_longer_than_mine ^ (!record), "if your chain is longer , so your reserved names should all be included in the chain ${r}, your chain is longer ${y}", ("r", your_name)("y", your_chain_longer_than_mine));
        }
        
        FC_ASSERT( my_chain->get_head_block_num() == your_chain->get_head_block_num() );
    }
    catch ( const fc::exception& e )
    {
        elog( "${e}", ("e",e.to_detail_string() ) );
        throw;
    }
}
