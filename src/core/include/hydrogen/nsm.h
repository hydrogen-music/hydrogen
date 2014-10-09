
/*************************************************************************/
/* Copyright (C) 2012 Jonathan Moore Liles                               */
/*                                                                       */
/* Permission to use, copy, modify, and/or distribute this software for  */
/* any purpose with or without fee is hereby granted, provided that the  */
/* above copyright notice and this permission notice appear in all       */
/* copies.                                                               */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL         */
/* WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED         */
/* WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE      */
/* AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL  */
/* DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR */
/* PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER        */
/* TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR      */
/* PERFORMANCE OF THIS SOFTWARE.                                         */
/*************************************************************************/


/*************************************************************/
/* A simple, callback based C API for NSM clients.           */
/*                                                           */
/* Simplified Example:                                       */
/*                                                           */
/* #include "nsm.h"                                          */
/*                                                           */
/* int                                                       */
/* cb_nsm_open ( const char *name,                           */
/*               const char *display_name,                   */
/*               const char *client_id,                      */
/*               char **out_msg,                             */
/*               void *userdata )                            */
/* {                                                         */
/*         do_open_stuff();                                  */
/*         return ERR_OK;                                    */
/* }                                                         */
/*                                                           */
/* int                                                       */
/* cb_nsm_save ( char **out_msg,                             */
/*               void *userdata )                            */
/* {                                                         */
/*     do_save_stuff();                                      */
/*     return ERR_OK;                                        */
/* }                                                         */
/*                                                           */
/* static nsm_client_t *nsm = 0                              */
/*                                                           */
/* int main( int argc, char **argv )                         */
/* {                                                         */
/*     const char *nsm_url = getenv( "NSM_URL" );            */
/*                                                           */
/*     if ( nsm_url )                                        */
/*     {                                                     */
/*         nsm = nsm_new();                                  */
/*                                                           */
/*         nsm_set_open_callback( nsm, cb_nsm_open, 0 );     */
/*         nsm_set_save_callback( nsm, cb_nsm_save, 0 );     */
/*                                                           */
/*         if ( 0 == nsm_init( nsm, nsm_url ) )              */
/*         {                                                 */
/*             nsm_send_announce( nsm, "FOO", "", argv[0] ); */
/*         }                                                 */
/*         else                                              */
/*         {                                                 */
/*             nsm_free( nsm );                              */
/*             nsm = 0;                                      */
/*         }                                                 */
/*     }                                                     */
/* }                                                         */
/*************************************************************/

#ifndef _NSM_H
#define _NSM_H

#define NSM_API_VERSION_MAJOR 1
#define NSM_API_VERSION_MINOR 0

#include <lo/lo.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

typedef void * nsm_client_t;
typedef int (nsm_open_callback)( const char *name, const char *display_name, const char *client_id, char **out_msg, void *userdata );
typedef int (nsm_save_callback)( char **out_msg, void *userdata );
typedef void (nsm_active_callback)( int b, void *userdata );
typedef void (nsm_session_is_loaded_callback)( void *userdata );
typedef int (nsm_broadcast_callback)( const char *, lo_message m, void *userdata );

#define _NSM() ((struct _nsm_client_t*)nsm)

#define NSM_EXPORT __attribute__((unused)) static

/* private parts */
struct _nsm_client_t
{
    const char *nsm_url;
    
    lo_server _server;
    lo_server_thread _st;
    lo_address nsm_addr;
    
    int nsm_is_active;
    char *nsm_client_id;
    char *_session_manager_name;

    nsm_open_callback *open;
    void *open_userdata;

    nsm_save_callback *save;
    void *save_userdata;

    nsm_active_callback *active;
    void *active_userdata;

    nsm_session_is_loaded_callback *session_is_loaded;
    void *session_is_loaded_userdata;

    nsm_broadcast_callback *broadcast;
    void *broadcast_userdata;
};

enum
{
    ERR_OK = 0,
    ERR_GENERAL    = -1,
    ERR_INCOMPATIBLE_API = -2,
    ERR_BLACKLISTED      = -3,
    ERR_LAUNCH_FAILED    = -4,
    ERR_NO_SUCH_FILE     = -5,
    ERR_NO_SESSION_OPEN  = -6,
    ERR_UNSAVED_CHANGES  = -7,
    ERR_NOT_NOW          = -8
};

NSM_EXPORT
int
nsm_is_active ( nsm_client_t *nsm )
{
    return _NSM()->nsm_is_active;
}

NSM_EXPORT
const char *
nsm_get_session_manager_name ( nsm_client_t *nsm )
{
    return _NSM()->_session_manager_name;
}

NSM_EXPORT
nsm_client_t *
nsm_new ( void )
{
    struct _nsm_client_t *nsm = (struct _nsm_client_t*)malloc( sizeof( struct _nsm_client_t ) );

    nsm->nsm_url = 0;

    nsm->nsm_is_active = 0;
    nsm->nsm_client_id = 0;
    
    nsm->_server = 0;
    nsm->_st = 0;
    nsm->nsm_addr = 0;
    nsm->_session_manager_name = 0;
    
    nsm->open = 0;
    nsm->save = 0;
    nsm->active = 0;
    nsm->session_is_loaded = 0;
    nsm->broadcast = 0;

    return (nsm_client_t *)nsm;
}

/*******************************************/
/* CLIENT TO SERVER INFORMATIONAL MESSAGES */
/*******************************************/

NSM_EXPORT 
void
nsm_send_is_dirty ( nsm_client_t *nsm )
{
    if ( _NSM()->nsm_is_active )
        lo_send_from( _NSM()->nsm_addr, _NSM()->_server, LO_TT_IMMEDIATE, "/nsm/client/is_dirty", "" );
}

NSM_EXPORT 
void
nsm_send_is_clean ( nsm_client_t *nsm )
{
    if ( _NSM()->nsm_is_active )
        lo_send_from( _NSM()->nsm_addr, _NSM()->_server, LO_TT_IMMEDIATE, "/nsm/client/is_clean", "" );
}

NSM_EXPORT 
void
nsm_send_progress ( nsm_client_t *nsm, float p )
{
    if ( _NSM()->nsm_is_active )
        lo_send_from( _NSM()->nsm_addr, _NSM()->_server, LO_TT_IMMEDIATE, "/nsm/client/progress", "f", p );
}

NSM_EXPORT 
void
nsm_send_message ( nsm_client_t *nsm, int priority, const char *msg )
{
   if ( _NSM()->nsm_is_active )
       lo_send_from( _NSM()->nsm_addr, _NSM()->_server, LO_TT_IMMEDIATE, "/nsm/client/message", "is", priority, msg );
}

NSM_EXPORT void
nsm_send_announce ( nsm_client_t *nsm, const char *app_name, const char *capabilities, const char *process_name )
{
    lo_address to = lo_address_new_from_url( _NSM()->nsm_url );
    
    if ( ! to )
    {
        fprintf( stderr, "NSM: Bad address!" );
        return;
    }
    
    int pid = (int)getpid();
    
    lo_send_from( to, _NSM()->_server, LO_TT_IMMEDIATE, "/nsm/server/announce", "sssiii",
                  app_name,
                  capabilities,
                  process_name,
                  NSM_API_VERSION_MAJOR,
                  NSM_API_VERSION_MINOR,
                  pid );
    
    lo_address_free( to );
}

NSM_EXPORT void 
nsm_send_broadcast ( nsm_client_t *nsm, lo_message msg )
{
   if ( _NSM()->nsm_is_active )
       lo_send_message_from( _NSM()->nsm_addr, _NSM()->_server, "/nsm/server/broadcast", msg );
}



NSM_EXPORT
void
nsm_check_wait ( nsm_client_t *nsm, int timeout )
{
    if ( lo_server_wait( _NSM()->_server, timeout ) )
        while ( lo_server_recv_noblock( _NSM()->_server, 0 ) ) {}
}

NSM_EXPORT
void
nsm_check_nowait (nsm_client_t *nsm )
{
    nsm_check_wait( nsm, 0 );
}


NSM_EXPORT
void
nsm_thread_start ( nsm_client_t *nsm )
{
    lo_server_thread_start( _NSM()->_st );
}


NSM_EXPORT
void
nsm_thread_stop ( nsm_client_t *nsm )
{
    lo_server_thread_stop( _NSM()->_st );
}



NSM_EXPORT void
nsm_free ( nsm_client_t *nsm )
{
    if ( _NSM()->_st )
        nsm_thread_stop( nsm );
    
    if ( _NSM()->_st )
        lo_server_thread_free( _NSM()->_st );
    else
        lo_server_free( _NSM()->_server );
    
    free( _NSM() );
}

/*****************/
/* SET CALLBACKS */
/*****************/

NSM_EXPORT
void
nsm_set_open_callback( nsm_client_t *nsm, nsm_open_callback *open_callback, void *userdata )
{
    _NSM()->open = open_callback;
    _NSM()->open_userdata = userdata;
}

NSM_EXPORT
void
nsm_set_save_callback( nsm_client_t *nsm, nsm_save_callback *save_callback, void *userdata )
{
    _NSM()->save = save_callback;
    _NSM()->save_userdata = userdata;
}

NSM_EXPORT
void
nsm_set_active_callback( nsm_client_t *nsm, nsm_active_callback *active_callback, void *userdata )
{
    _NSM()->active = active_callback;
    _NSM()->active_userdata = userdata;
}

NSM_EXPORT
void
nsm_set_session_is_loaded_callback( nsm_client_t *nsm, nsm_session_is_loaded_callback *session_is_loaded_callback, void *userdata )
{
    _NSM()->session_is_loaded = session_is_loaded_callback;
    _NSM()->session_is_loaded_userdata = userdata;
}


NSM_EXPORT
void
nsm_set_broadcast_callback( nsm_client_t *nsm, nsm_broadcast_callback *broadcast_callback, void *userdata )
{
    _NSM()->broadcast = broadcast_callback;
    _NSM()->broadcast_userdata = userdata;
}



/****************/
/* OSC HANDLERS */
/****************/

#undef OSC_REPLY
#undef OSC_REPLY_ERR

#define OSC_REPLY( value ) lo_send_from( ((struct _nsm_client_t*)user_data)->nsm_addr, ((struct _nsm_client_t*)user_data)->_server, LO_TT_IMMEDIATE, "/reply", "ss", path, value )
#define OSC_REPLY_P( path, value ) lo_send_from( ((struct _nsm_client_t*)user_data)->nsm_addr, ((struct _nsm_client_t*)user_data)->_server, LO_TT_IMMEDIATE, "/reply", "ss", path, value )

#define OSC_REPLY_ERR( errcode, value ) lo_send_from( ((struct _nsm_client_t*)user_data)->nsm_addr, ((struct _nsm_client_t*)user_data)->_server, LO_TT_IMMEDIATE, "/error", "sis", path, errcode, value )


NSM_EXPORT int _nsm_osc_open ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    (void) types;
    (void) argc;
    (void) msg;

    char *out_msg = NULL;
    
    struct _nsm_client_t *nsm = (struct _nsm_client_t*)user_data;
    
    nsm->nsm_client_id = strdup( &argv[2]->s );
    
    if ( ! nsm->open )
        return 0;

    int r = nsm->open( &argv[0]->s, &argv[1]->s, &argv[2]->s, &out_msg, nsm->open_userdata );
    
    if ( r )
        OSC_REPLY_ERR( r, ( out_msg ? out_msg : "") );
    else
        OSC_REPLY( "OK" );
    
    if ( out_msg )
        free( out_msg );
    
    return 0;
}

NSM_EXPORT int _nsm_osc_save ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    (void) types;
    (void) argv;
    (void) argc;
    (void) msg;

    char *out_msg = NULL;
    
    struct _nsm_client_t *nsm = (struct _nsm_client_t*)user_data;

    if ( ! nsm->save )
        return 0;

    int r = nsm->save(&out_msg, nsm->save_userdata );
    
    if ( r )
    {
        OSC_REPLY_ERR( r, ( out_msg ? out_msg : "") );
    }
    else
    {
        OSC_REPLY( "OK" );
    }
    if ( out_msg )
        free( out_msg );
    
    return 0;
}

NSM_EXPORT int _nsm_osc_announce_reply ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    (void) path;
    (void) types;
    (void) argc;

    if ( strcmp( &argv[0]->s, "/nsm/server/announce" ) )
        return -1;
    
    struct _nsm_client_t *nsm = (struct _nsm_client_t*)user_data;

    fprintf( stderr, "NSM: Successfully registered. NSM says: %s", &argv[1]->s );

    nsm->nsm_is_active = 1;
    nsm->_session_manager_name = strdup( &argv[2]->s );
    nsm->nsm_addr = lo_address_new_from_url( lo_address_get_url( lo_message_get_source( msg ) ));   
    
    if ( nsm->active )
        nsm->active( nsm->nsm_is_active, nsm->active_userdata );
    
    return 0;
}

NSM_EXPORT int _nsm_osc_error ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    (void) path;
    (void) types;
    (void) argc;
    (void) msg;

    if ( strcmp( &argv[0]->s, "/nsm/server/announce" ) )
        return -1;

    struct _nsm_client_t *nsm = (struct _nsm_client_t*)user_data;

    fprintf( stderr, "NSM: Failed to register with NSM server: %s", &argv[2]->s );

    nsm->nsm_is_active = 0;
        
    if ( nsm->active )
        nsm->active( nsm->nsm_is_active, nsm->active_userdata );

    return 0;
}

NSM_EXPORT int _nsm_osc_session_is_loaded ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    (void) path;
    (void) types;
    (void) argv;
    (void) argc;
    (void) msg;

    struct _nsm_client_t *nsm = (struct _nsm_client_t*)user_data;

    if ( ! nsm->session_is_loaded )
        return 0;
    
    nsm->session_is_loaded( nsm->session_is_loaded_userdata );

    return 0;
}

NSM_EXPORT int _nsm_osc_broadcast ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    (void) types;
    (void) argv;
    (void) argc;

    struct _nsm_client_t *nsm = (struct _nsm_client_t*)user_data;

    if ( ! nsm->broadcast )
        return 0;
    
    return nsm->broadcast( path, msg, nsm->broadcast_userdata );
}



NSM_EXPORT
int
nsm_init ( nsm_client_t *nsm, const char *nsm_url )
{
    _NSM()->nsm_url = nsm_url;

    lo_address addr = lo_address_new_from_url( nsm_url );
    int proto = lo_address_get_protocol( addr );
    lo_address_free( addr );

    _NSM()->_server = lo_server_new_with_proto( NULL, proto, NULL );

    if ( ! _NSM()->_server )
        return -1;

    lo_server_add_method( _NSM()->_server, "/error", "sis", _nsm_osc_error, _NSM() );
    lo_server_add_method( _NSM()->_server, "/reply", "ssss", _nsm_osc_announce_reply, _NSM() );
    lo_server_add_method( _NSM()->_server, "/nsm/client/open", "sss", _nsm_osc_open, _NSM() );
    lo_server_add_method( _NSM()->_server, "/nsm/client/save", "", _nsm_osc_save, _NSM() );
    lo_server_add_method( _NSM()->_server, "/nsm/client/session_is_loaded", "", _nsm_osc_session_is_loaded, _NSM() );
    lo_server_add_method( _NSM()->_server, NULL, NULL, _nsm_osc_broadcast, _NSM() );

    return 0;
}


NSM_EXPORT
int
nsm_init_thread ( nsm_client_t *nsm, const char *nsm_url )
{
    _NSM()->nsm_url = nsm_url;

    lo_address addr = lo_address_new_from_url( nsm_url );
    int proto = lo_address_get_protocol( addr );
    lo_address_free( addr );

    _NSM()->_st = lo_server_thread_new_with_proto( NULL, proto, NULL );
    _NSM()->_server = lo_server_thread_get_server( _NSM()->_st );
    
    if ( ! _NSM()->_server )
        return -1;

    lo_server_thread_add_method( _NSM()->_st, "/error", "sis", _nsm_osc_error, _NSM() );
    lo_server_thread_add_method( _NSM()->_st, "/reply", "ssss", _nsm_osc_announce_reply, _NSM() );
    lo_server_thread_add_method( _NSM()->_st, "/nsm/client/open", "sss", _nsm_osc_open, _NSM() );
    lo_server_thread_add_method( _NSM()->_st, "/nsm/client/save", "", _nsm_osc_save, _NSM() );
    lo_server_thread_add_method( _NSM()->_st, "/nsm/client/session_is_loaded", "", _nsm_osc_session_is_loaded, _NSM() );
    lo_server_thread_add_method( _NSM()->_st, NULL, NULL, _nsm_osc_broadcast, _NSM() );

    return 0;
}

#endif /* NSM_H */
