/*
   Copyright (c) 2011, 2017 Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "ndb_mi.h"
#include "ha_ndbcluster_glue.h"
#include "my_sys.h"
#include "hash.h"
#include "rpl_mi.h"

#ifdef HAVE_NDB_BINLOG

extern Master_info *active_mi;


uint32 ndb_mi_get_master_server_id()
{
  DBUG_ASSERT (active_mi != NULL);
  return (uint32) active_mi->master_id;
}

const char* ndb_mi_get_group_master_log_name()
{
  DBUG_ASSERT (active_mi != NULL);
#if MYSQL_VERSION_ID < 50600
  return active_mi->rli.group_master_log_name;
#else
  return active_mi->rli->get_group_master_log_name();
#endif
}

uint64 ndb_mi_get_group_master_log_pos()
{
  DBUG_ASSERT (active_mi != NULL);
#if MYSQL_VERSION_ID < 50600
  return (uint64) active_mi->rli.group_master_log_pos;
#else
  return (uint64) active_mi->rli->get_group_master_log_pos();
#endif
}

uint64 ndb_mi_get_future_event_relay_log_pos()
{
  DBUG_ASSERT (active_mi != NULL);
#if MYSQL_VERSION_ID < 50600
  return (uint64) active_mi->rli.future_event_relay_log_pos;
#else
  return (uint64) active_mi->rli->get_future_event_relay_log_pos();
#endif
}

uint64 ndb_mi_get_group_relay_log_pos()
{
  DBUG_ASSERT (active_mi != NULL);
#if MYSQL_VERSION_ID < 50600
  return (uint64) active_mi->rli.group_relay_log_pos;
#else
  return (uint64) active_mi->rli->get_group_relay_log_pos();
#endif
}

bool ndb_mi_get_ignore_server_id(uint32 server_id)
{
  DBUG_ASSERT (active_mi != NULL);
  return (active_mi->shall_ignore_server_id(server_id) != 0);
}

uint32 ndb_mi_get_slave_run_id()
{
  DBUG_ASSERT (active_mi != NULL);
#if MYSQL_VERSION_ID < 50600
  return active_mi->rli.slave_run_id;
#else
  return active_mi->rli->slave_run_id;
#endif
}

ulong ndb_mi_get_relay_log_trans_retries()
{
  DBUG_ASSERT (active_mi != NULL);
#if MYSQL_VERSION_ID < 50600
  return active_mi->rli.trans_retries;
#else
  return active_mi->rli->trans_retries;
#endif
}

void ndb_mi_set_relay_log_trans_retries(ulong number)
{
  DBUG_ASSERT (active_mi != NULL);
#if MYSQL_VERSION_ID < 50600
  active_mi->rli.trans_retries = number;
#else
  active_mi->rli->trans_retries = number;
#endif
}

ulong ndb_mi_get_slave_parallel_workers()
{
  DBUG_ASSERT (active_mi != NULL);
#if MYSQL_VERSION_ID < 50600
  return active_mi->rli.opt_slave_parallel_workers;
#else
  return active_mi->rli->opt_slave_parallel_workers;
#endif
}

/* #ifdef HAVE_NDB_BINLOG */

#endif