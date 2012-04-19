/* packet-gluster.c
 * Routines for gluster dissection
 * Copyright 2012, Niels de Vos <ndevos@redhat.com>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *
 * References to source files point in general to the glusterfs sources.
 * There is currently no RFC or other document where the protocol is
 * completely described. The glusterfs sources can be found at:
 * - http://git.gluster.com/?p=glusterfs.git
 * - https://github.com/gluster/glusterfs
 *
 * The coding-style is roughly the same as the one use in the Linux kernel,
 * see http://www.kernel.org/doc/Documentation/CodingStyle.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <string.h>
#include <epan/packet.h>
#include <epan/tfs.h>

#include "packet-rpc.h"
#include "packet-gluster.h"

/* Initialize the protocol and registered fields */
gint proto_gluster = -1;
static gint proto_gluster_mgmt = -1;
static gint proto_gd_mgmt = -1;
static gint proto_gluster_cli = -1;
static gint proto_gluster_cbk = -1;

/* programs and procedures */
static gint hf_gluster_mgmt_proc = -1;
static gint hf_gd_mgmt_proc = -1;
static gint hf_gluster_cli_proc = -1;
static gint hf_gluster_cbk_proc = -1;

/* fields used by multiple programs/procedures */
gint hf_gluster_gfid = -1;
gint hf_gluster_op = -1;
gint hf_gluster_op_ret = -1;
gint hf_gluster_op_errno = -1;
static gint hf_gluster_op_errstr = -1;
static gint hf_gluster_dict_key = -1;
static gint hf_gluster_dict_value = -1;

static gint hf_gluster_uuid = -1;
static gint hf_gluster_hostname = -1;
static gint hf_gluster_port = -1;
static gint hf_gluster_vols = -1;

/* temporarily used during development */
static gint hf_gluster_dict = -1;
static gint hf_gluster_unknown_int = -1;

/* Initialize the subtree pointers */
static gint ett_gluster = -1;
static gint ett_gluster_mgmt = -1;
static gint ett_gd_mgmt = -1;
static gint ett_gluster_cli = -1;
static gint ett_gluster_cbk = -1;
static gint ett_gluster_dict = -1;

/* function for dissecting and adding a gluster dict_t to the tree */
int
gluster_rpc_dissect_dict(proto_tree *tree, tvbuff_t *tvb /* FIXME: add ", int hfindex" */, int offset)
{
	gchar *key, *value;
	gint items, i, len, roundup, value_len, key_len;

	proto_item *dict_item;
	proto_tree *dict_tree;

	len = tvb_get_ntohl(tvb, offset);
	roundup = rpc_roundup(len) - len;
	proto_tree_add_text(tree, tvb, offset, 4, "Size of the dict: %d (%d bytes inc. RPC-roundup)", len, rpc_roundup(len));
	offset += 4;

	if (len == 0)
		return offset;

	items = tvb_get_ntohl(tvb, offset);
	proto_tree_add_text(tree, tvb, offset, 4, "Items in the dict: %d", items);
	offset += 4;

	for (i = 0; i < items; i++) {
		dict_item = proto_tree_add_text(tree, tvb, offset, -1, "Item %d", i);
		dict_tree = proto_item_add_subtree(dict_item, ett_gluster_dict);

		/* key_len is the length of the key without the terminating '\0' */
		/* key_len = tvb_get_ntohl(tvb, offset) + 1; // will be read later */
		offset += 4;
		value_len = tvb_get_ntohl(tvb, offset);
		offset += 4;

		/* read the key, '\0' terminated */
		key = tvb_get_stringz(tvb, offset, &key_len);
		if (tree)
			proto_tree_add_string(dict_tree, hf_gluster_dict_key, tvb, offset, key_len, key);
		offset += key_len;
		g_free(key);

		/* read the value, '\0' terminated */
		value = tvb_get_string(tvb, offset, value_len);
		if (tree)
			proto_tree_add_string(dict_tree, hf_gluster_dict_value, tvb, offset, value_len, value);
		offset += value_len;
		g_free(value);
	}

	if (roundup) {
		if (tree)
			proto_tree_add_text(tree, tvb, offset, -1, "RPC-roundup bytes: %d", roundup);
		offset += roundup;
	}

	return offset;
}

int
gluster_dissect_rpc_uquad_t(tvbuff_t *tvb, proto_tree *tree, int hfindex, int offset)
{
	offset = dissect_rpc_uint64(tvb, tree, hfindex, offset);
	return offset;
}

static int
gluster_gd_mgmt_probe_reply(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *hostname = NULL;

	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_uuid, offset, 16 * 4, FALSE, NULL);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_hostname, offset, &hostname);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_port, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);

	return offset;
}

static int
gluster_gd_mgmt_probe_call(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *hostname = NULL;

	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_uuid, offset, 16 * 4, FALSE, NULL);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_hostname, offset, &hostname);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_port, offset);

	return offset;
}

static int
gluster_gd_mgmt_friend_add_reply(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *hostname = NULL;

	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_uuid, offset, 16 * 4, FALSE, NULL);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_hostname, offset, &hostname);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_port, offset);

	return offset;
}

static int
gluster_gd_mgmt_friend_add_call(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *hostname = NULL;

	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_uuid, offset, 16 * 4, FALSE, NULL);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_hostname, offset, &hostname);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_port, offset);
	/* FIXME: how to call this one? vols? */
	offset = gluster_rpc_dissect_dict(tree, tvb, offset);

	return offset;
}

/* gluster_gd_mgmt_cluster_lock_reply is used for LOCK and UNLOCK */
static int
gluster_gd_mgmt_cluster_lock_reply(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_uuid, offset, 16 * 4, FALSE, NULL);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);

	return offset;
}

/* gluster_gd_mgmt_cluster_lock_call is used for LOCK and UNLOCK */
static int
gluster_gd_mgmt_cluster_lock_call(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_uuid, offset, 16 * 4, FALSE, NULL);

	return offset;
}

static int
gluster_gd_mgmt_stage_op_reply(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *errstr = NULL;

	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_uuid, offset, 16 * 4, FALSE, NULL);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_op_errstr, offset, &errstr);
	offset = gluster_rpc_dissect_dict(tree, tvb, offset);
	return offset;
}

static int
gluster_gd_mgmt_stage_op_call(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_uuid, offset, 16 * 4, FALSE, NULL);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, offset);

	return offset;
}

static int
gluster_gd_mgmt_commit_op_reply(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *errstr = NULL;

	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_uuid, offset, 16 * 4, FALSE, NULL);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, offset);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_op_errstr, offset, &errstr);
	return offset;
}

static int
gluster_gd_mgmt_commit_op_call(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_uuid, offset, 16 * 4, FALSE, NULL);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, offset);

	return offset;
}

static int
gluster_gd_mgmt_friend_update_reply(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_uuid, offset, 16 * 4, FALSE, NULL);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);

	return offset;
}

static int
gluster_gd_mgmt_friend_update_call(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_uuid, offset, 16 * 4, FALSE, NULL);
	/* FIXME: how to call this one? vols? */
	offset = gluster_rpc_dissect_dict(tree, tvb, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_port, offset);

	return offset;
}

/* GLUSTERD1_MGMT_PROGRAM from xlators/mgmt/glusterd/src/glusterd-rpc-ops.c */
static const vsff gluster_mgmt_proc[] = {
	{ GLUSTERD_MGMT_NULL, "NULL", NULL, NULL },
	{ GLUSTERD_MGMT_PROBE_QUERY, "PROBE_QUERY", NULL, NULL },
	{ GLUSTERD_MGMT_FRIEND_ADD, "FRIEND_ADD", NULL, NULL },
	{ GLUSTERD_MGMT_CLUSTER_LOCK, "CLUSTER_LOCK", NULL, NULL },
	{ GLUSTERD_MGMT_CLUSTER_UNLOCK, "CLUSTER_UNLOCK", NULL, NULL },
	{ GLUSTERD_MGMT_STAGE_OP, "STAGE_OP", NULL, NULL },
	{ GLUSTERD_MGMT_COMMIT_OP, "COMMIT_OP", NULL, NULL },
	{ GLUSTERD_MGMT_FRIEND_REMOVE, "FRIEND_REMOVE", NULL, NULL },
	{ GLUSTERD_MGMT_FRIEND_UPDATE, "FRIEND_UPDATE", NULL, NULL },
	{ 0, NULL, NULL, NULL }
};
static const value_string gluster_mgmt_proc_vals[] = {
	{ GLUSTERD_MGMT_NULL, "NULL" },
	{ GLUSTERD_MGMT_PROBE_QUERY, "PROBE_QUERY" },
	{ GLUSTERD_MGMT_FRIEND_ADD, "FRIEND_ADD" },
	{ GLUSTERD_MGMT_CLUSTER_LOCK, "CLUSTER_LOCK" },
	{ GLUSTERD_MGMT_CLUSTER_UNLOCK, "CLUSTER_UNLOCK" },
	{ GLUSTERD_MGMT_STAGE_OP, "STAGE_OP" },
	{ GLUSTERD_MGMT_COMMIT_OP, "COMMIT_OP" },
	{ GLUSTERD_MGMT_FRIEND_REMOVE, "FRIEND_REMOVE" },
	{ GLUSTERD_MGMT_FRIEND_UPDATE, "FRIEND_UPDATE" },
	{ 0, NULL }
};

/*
 * GD_MGMT_PROGRAM
 * - xlators/mgmt/glusterd/src/glusterd-handler.c: "GlusterD svc mgmt"
 * - xlators/mgmt/glusterd/src/glusterd-rpc-ops.c: "glusterd clnt mgmt"
 */
static const vsff gd_mgmt_proc[] = {
	{ GD_MGMT_NULL, "NULL", NULL, NULL},
	{
		GD_MGMT_PROBE_QUERY, "GD_MGMT_PROBE_QUERY",
		gluster_gd_mgmt_probe_call, gluster_gd_mgmt_probe_reply
	},
	{
		GD_MGMT_FRIEND_ADD, "GD_MGMT_FRIEND_ADD",
		gluster_gd_mgmt_friend_add_call, gluster_gd_mgmt_friend_add_reply
	},
	{
		GD_MGMT_CLUSTER_LOCK, "GD_MGMT_CLUSTER_LOCK",
		gluster_gd_mgmt_cluster_lock_call, gluster_gd_mgmt_cluster_lock_reply
	},
	{
		GD_MGMT_CLUSTER_UNLOCK, "GD_MGMT_CLUSTER_UNLOCK",
		/* UNLOCK seems to be the same a LOCK, re-use the function */
		gluster_gd_mgmt_cluster_lock_call, gluster_gd_mgmt_cluster_lock_reply
	},
	{
		GD_MGMT_STAGE_OP, "GD_MGMT_STAGE_OP",
		gluster_gd_mgmt_stage_op_call, gluster_gd_mgmt_stage_op_reply
	},
	{
		GD_MGMT_COMMIT_OP, "GD_MGMT_COMMIT_OP",
		gluster_gd_mgmt_commit_op_call, gluster_gd_mgmt_commit_op_reply
	},
	{ GD_MGMT_FRIEND_REMOVE, "GD_MGMT_FRIEND_REMOVE", NULL, NULL},
	{
		GD_MGMT_FRIEND_UPDATE, "GD_MGMT_FRIEND_UPDATE",
		gluster_gd_mgmt_friend_update_call, gluster_gd_mgmt_friend_update_reply
	},
	{ GD_MGMT_CLI_PROBE, "GD_MGMT_CLI_PROBE", NULL, NULL},
	{ GD_MGMT_CLI_DEPROBE, "GD_MGMT_CLI_DEPROBE", NULL, NULL},
	{ GD_MGMT_CLI_LIST_FRIENDS, "GD_MGMT_CLI_LIST_FRIENDS", NULL, NULL},
	{ GD_MGMT_CLI_CREATE_VOLUME, "GD_MGMT_CLI_CREATE_VOLUME", NULL, NULL},
	{ GD_MGMT_CLI_GET_VOLUME, "GD_MGMT_CLI_GET_VOLUME", NULL, NULL},
	{ GD_MGMT_CLI_DELETE_VOLUME, "GD_MGMT_CLI_DELETE_VOLUME", NULL, NULL},
	{ GD_MGMT_CLI_START_VOLUME, "GD_MGMT_CLI_START_VOLUME", NULL, NULL},
	{ GD_MGMT_CLI_STOP_VOLUME, "GD_MGMT_CLI_STOP_VOLUME", NULL, NULL},
	{ GD_MGMT_CLI_RENAME_VOLUME, "GD_MGMT_CLI_RENAME_VOLUME", NULL, NULL},
	{ GD_MGMT_CLI_DEFRAG_VOLUME, "GD_MGMT_CLI_DEFRAG_VOLUME", NULL, NULL},
	{ GD_MGMT_CLI_SET_VOLUME, "GD_MGMT_CLI_DEFRAG_VOLUME", NULL, NULL},
	{ GD_MGMT_CLI_ADD_BRICK, "GD_MGMT_CLI_ADD_BRICK", NULL, NULL},
	{ GD_MGMT_CLI_REMOVE_BRICK, "GD_MGMT_CLI_REMOVE_BRICK", NULL, NULL},
	{ GD_MGMT_CLI_REPLACE_BRICK, "GD_MGMT_CLI_REPLACE_BRICK", NULL, NULL},
	{ GD_MGMT_CLI_LOG_FILENAME, "GD_MGMT_CLI_LOG_FILENAME", NULL, NULL},
	{ GD_MGMT_CLI_LOG_LOCATE, "GD_MGMT_CLI_LOG_LOCATE", NULL, NULL},
	{ GD_MGMT_CLI_LOG_ROTATE, "GD_MGMT_CLI_LOG_ROTATE", NULL, NULL},
	{ GD_MGMT_CLI_SYNC_VOLUME, "GD_MGMT_CLI_SYNC_VOLUME", NULL, NULL},
	{ GD_MGMT_CLI_RESET_VOLUME, "GD_MGMT_CLI_RESET_VOLUME", NULL, NULL},
	{ GD_MGMT_CLI_FSM_LOG, "GD_MGMT_CLI_FSM_LOG", NULL, NULL},
	{ GD_MGMT_CLI_GSYNC_SET, "GD_MGMT_CLI_GSYNC_SET", NULL, NULL},
	{ GD_MGMT_CLI_PROFILE_VOLUME, "GD_MGMT_CLI_PROFILE_VOLUME", NULL, NULL},
	{ GD_MGMT_BRICK_OP, "BRICK_OP", NULL, NULL},
	{ GD_MGMT_CLI_LOG_LEVEL, "GD_MGMT_CLI_LOG_LEVEL", NULL, NULL},
	{ GD_MGMT_CLI_STATUS_VOLUME, "GD_MGMT_CLI_STATUS_VOLUME", NULL, NULL},
	{ GD_MGMT_MAXVALUE, "GD_MGMT_MAXVALUE", NULL, NULL},
	{ 0, NULL, NULL, NULL}
};
static const value_string gd_mgmt_proc_vals[] = {
	{ GD_MGMT_NULL, "NULL" },
	{ GD_MGMT_PROBE_QUERY, "GD_MGMT_PROBE_QUERY" },
	{ GD_MGMT_FRIEND_ADD, "GD_MGMT_FRIEND_ADD" },
	{ GD_MGMT_CLUSTER_LOCK, "GD_MGMT_CLUSTER_LOCK" },
	{ GD_MGMT_CLUSTER_UNLOCK, "GD_MGMT_CLUSTER_UNLOCK" },
	{ GD_MGMT_STAGE_OP, "GD_MGMT_STAGE_OP" },
	{ GD_MGMT_COMMIT_OP, "GD_MGMT_COMMIT_OP" },
	{ GD_MGMT_FRIEND_REMOVE, "GD_MGMT_FRIEND_REMOVE" },
	{ GD_MGMT_FRIEND_UPDATE, "GD_MGMT_FRIEND_UPDATE" },
	{ GD_MGMT_CLI_PROBE, "GD_MGMT_CLI_PROBE" },
	{ GD_MGMT_CLI_DEPROBE, "GD_MGMT_CLI_DEPROBE" },
	{ GD_MGMT_CLI_LIST_FRIENDS, "GD_MGMT_CLI_LIST_FRIENDS" },
	{ GD_MGMT_CLI_CREATE_VOLUME, "GD_MGMT_CLI_CREATE_VOLUME" },
	{ GD_MGMT_CLI_GET_VOLUME, "GD_MGMT_CLI_GET_VOLUME" },
	{ GD_MGMT_CLI_DELETE_VOLUME, "GD_MGMT_CLI_DELETE_VOLUME" },
	{ GD_MGMT_CLI_START_VOLUME, "GD_MGMT_CLI_START_VOLUME" },
	{ GD_MGMT_CLI_STOP_VOLUME, "GD_MGMT_CLI_STOP_VOLUME" },
	{ GD_MGMT_CLI_RENAME_VOLUME, "GD_MGMT_CLI_RENAME_VOLUME" },
	{ GD_MGMT_CLI_DEFRAG_VOLUME, "GD_MGMT_CLI_DEFRAG_VOLUME" },
	{ GD_MGMT_CLI_SET_VOLUME, "GD_MGMT_CLI_DEFRAG_VOLUME" },
	{ GD_MGMT_CLI_ADD_BRICK, "GD_MGMT_CLI_ADD_BRICK" },
	{ GD_MGMT_CLI_REMOVE_BRICK, "GD_MGMT_CLI_REMOVE_BRICK" },
	{ GD_MGMT_CLI_REPLACE_BRICK, "GD_MGMT_CLI_REPLACE_BRICK" },
	{ GD_MGMT_CLI_LOG_FILENAME, "GD_MGMT_CLI_LOG_FILENAME" },
	{ GD_MGMT_CLI_LOG_LOCATE, "GD_MGMT_CLI_LOG_LOCATE" },
	{ GD_MGMT_CLI_LOG_ROTATE, "GD_MGMT_CLI_LOG_ROTATE" },
	{ GD_MGMT_CLI_SYNC_VOLUME, "GD_MGMT_CLI_SYNC_VOLUME" },
	{ GD_MGMT_CLI_RESET_VOLUME, "GD_MGMT_CLI_RESET_VOLUME" },
	{ GD_MGMT_CLI_FSM_LOG, "GD_MGMT_CLI_FSM_LOG" },
	{ GD_MGMT_CLI_GSYNC_SET, "GD_MGMT_CLI_GSYNC_SET" },
	{ GD_MGMT_CLI_PROFILE_VOLUME, "GD_MGMT_CLI_PROFILE_VOLUME" },
	{ GD_MGMT_BRICK_OP, "BRICK_OP" },
	{ GD_MGMT_CLI_LOG_LEVEL, "GD_MGMT_CLI_LOG_LEVEL" },
	{ GD_MGMT_CLI_STATUS_VOLUME, "GD_MGMT_CLI_STATUS_VOLUME" },
	{ GD_MGMT_MAXVALUE, "GD_MGMT_MAXVALUE" },
	{ 0, NULL }
};

/* procedures for GLUSTER_CLI_PROGRAM */
static const vsff gluster_cli_proc[] = {
	{ GLUSTER_CLI_NULL, "GLUSTER_CLI_NULL", NULL, NULL },
	{ GLUSTER_CLI_PROBE, "GLUSTER_CLI_PROBE", NULL, NULL },
	{ GLUSTER_CLI_DEPROBE, "GLUSTER_CLI_DEPROBE", NULL, NULL },
	{ GLUSTER_CLI_LIST_FRIENDS, "GLUSTER_CLI_LIST_FRIENDS", NULL, NULL },
	{ GLUSTER_CLI_CREATE_VOLUME, "GLUSTER_CLI_CREATE_VOLUME", NULL, NULL },
	{ GLUSTER_CLI_GET_VOLUME, "GLUSTER_CLI_GET_VOLUME", NULL, NULL },
	{
		GLUSTER_CLI_GET_NEXT_VOLUME, "GLUSTER_CLI_GET_NEXT_VOLUME",
		NULL, NULL
	},
	{ GLUSTER_CLI_DELETE_VOLUME, "GLUSTER_CLI_DELETE_VOLUME", NULL, NULL },
	{ GLUSTER_CLI_START_VOLUME, "GLUSTER_CLI_START_VOLUME", NULL, NULL },
	{ GLUSTER_CLI_STOP_VOLUME, "GLUSTER_CLI_STOP_VOLUME", NULL, NULL },
	{ GLUSTER_CLI_RENAME_VOLUME, "GLUSTER_CLI_RENAME_VOLUME", NULL, NULL },
	{ GLUSTER_CLI_DEFRAG_VOLUME, "GLUSTER_CLI_DEFRAG_VOLUME", NULL, NULL },
	{ GLUSTER_CLI_SET_VOLUME, "GLUSTER_CLI_SET_VOLUME", NULL, NULL },
	{ GLUSTER_CLI_ADD_BRICK, "GLUSTER_CLI_ADD_BRICK", NULL, NULL },
	{ GLUSTER_CLI_REMOVE_BRICK, "GLUSTER_CLI_REMOVE_BRICK", NULL, NULL },
	{ GLUSTER_CLI_REPLACE_BRICK, "GLUSTER_CLI_REPLACE_BRICK", NULL, NULL },
	{ GLUSTER_CLI_LOG_FILENAME, "GLUSTER_CLI_LOG_FILENAME", NULL, NULL },
	{ GLUSTER_CLI_LOG_LOCATE, "GLUSTER_CLI_LOG_LOCATE", NULL, NULL },
	{ GLUSTER_CLI_LOG_ROTATE, "GLUSTER_CLI_LOG_ROTATE", NULL, NULL },
	{ GLUSTER_CLI_GETSPEC, "GLUSTER_CLI_GETSPEC", NULL, NULL },
	{
		GLUSTER_CLI_PMAP_PORTBYBRICK, "GLUSTER_CLI_PMAP_PORTBYBRICK",
		NULL , NULL
	},
	{ GLUSTER_CLI_SYNC_VOLUME, "GLUSTER_CLI_SYNC_VOLUME", NULL, NULL },
	{ GLUSTER_CLI_RESET_VOLUME, "GLUSTER_CLI_RESET_VOLUME", NULL, NULL },
	{ GLUSTER_CLI_FSM_LOG, "GLUSTER_CLI_FSM_LOG", NULL, NULL },
	{ GLUSTER_CLI_GSYNC_SET, "GLUSTER_CLI_GSYNC_SET", NULL, NULL },
	{
		GLUSTER_CLI_PROFILE_VOLUME, "GLUSTER_CLI_PROFILE_VOLUME",
		NULL, NULL
	},
	{ GLUSTER_CLI_QUOTA, "GLUSTER_CLI_QUOTA", NULL, NULL },
	{ GLUSTER_CLI_TOP_VOLUME, "GLUSTER_CLI_TOP_VOLUME", NULL, NULL },
	{ GLUSTER_CLI_GETWD, "GLUSTER_CLI_GETWD", NULL, NULL },
	{ GLUSTER_CLI_LOG_LEVEL, "GLUSTER_CLI_LOG_LEVEL", NULL, NULL },
	{ GLUSTER_CLI_STATUS_VOLUME, "GLUSTER_CLI_STATUS_VOLUME", NULL, NULL },
	{ GLUSTER_CLI_MOUNT, "GLUSTER_CLI_MOUNT", NULL, NULL },
	{ GLUSTER_CLI_UMOUNT, "GLUSTER_CLI_UMOUNT", NULL, NULL },
	{ GLUSTER_CLI_HEAL_VOLUME, "GLUSTER_CLI_HEAL_VOLUME", NULL, NULL },
	{
		GLUSTER_CLI_STATEDUMP_VOLUME, "GLUSTER_CLI_STATEDUMP_VOLUME",
		NULL, NULL
	},
	{ GLUSTER_CLI_MAXVALUE, "GLUSTER_CLI_MAXVALUE", NULL, NULL },
	{ 0, NULL, NULL, NULL }
};
static const value_string gluster_cli_proc_vals[] = {
	{ GLUSTER_CLI_NULL, "GLUSTER_CLI_NULL" },
	{ GLUSTER_CLI_PROBE, "GLUSTER_CLI_PROBE" },
	{ GLUSTER_CLI_DEPROBE, "GLUSTER_CLI_DEPROBE" },
	{ GLUSTER_CLI_LIST_FRIENDS, "GLUSTER_CLI_LIST_FRIENDS" },
	{ GLUSTER_CLI_CREATE_VOLUME, "GLUSTER_CLI_CREATE_VOLUME" },
	{ GLUSTER_CLI_GET_VOLUME, "GLUSTER_CLI_GET_VOLUME" },
	{ GLUSTER_CLI_GET_NEXT_VOLUME, "GLUSTER_CLI_GET_NEXT_VOLUME" },
	{ GLUSTER_CLI_DELETE_VOLUME, "GLUSTER_CLI_DELETE_VOLUME" },
	{ GLUSTER_CLI_START_VOLUME, "GLUSTER_CLI_START_VOLUME" },
	{ GLUSTER_CLI_STOP_VOLUME, "GLUSTER_CLI_STOP_VOLUME" },
	{ GLUSTER_CLI_RENAME_VOLUME, "GLUSTER_CLI_RENAME_VOLUME" },
	{ GLUSTER_CLI_DEFRAG_VOLUME, "GLUSTER_CLI_DEFRAG_VOLUME" },
	{ GLUSTER_CLI_SET_VOLUME, "GLUSTER_CLI_SET_VOLUME" },
	{ GLUSTER_CLI_ADD_BRICK, "GLUSTER_CLI_ADD_BRICK" },
	{ GLUSTER_CLI_REMOVE_BRICK, "GLUSTER_CLI_REMOVE_BRICK" },
	{ GLUSTER_CLI_REPLACE_BRICK, "GLUSTER_CLI_REPLACE_BRICK" },
	{ GLUSTER_CLI_LOG_FILENAME, "GLUSTER_CLI_LOG_FILENAME" },
	{ GLUSTER_CLI_LOG_LOCATE, "GLUSTER_CLI_LOG_LOCATE" },
	{ GLUSTER_CLI_LOG_ROTATE, "GLUSTER_CLI_LOG_ROTATE" },
	{ GLUSTER_CLI_GETSPEC, "GLUSTER_CLI_GETSPEC" },
	{ GLUSTER_CLI_PMAP_PORTBYBRICK, "GLUSTER_CLI_PMAP_PORTBYBRICK" },
	{ GLUSTER_CLI_SYNC_VOLUME, "GLUSTER_CLI_SYNC_VOLUME" },
	{ GLUSTER_CLI_RESET_VOLUME, "GLUSTER_CLI_RESET_VOLUME" },
	{ GLUSTER_CLI_FSM_LOG, "GLUSTER_CLI_FSM_LOG" },
	{ GLUSTER_CLI_GSYNC_SET, "GLUSTER_CLI_GSYNC_SET" },
	{ GLUSTER_CLI_PROFILE_VOLUME, "GLUSTER_CLI_PROFILE_VOLUME" },
	{ GLUSTER_CLI_QUOTA, "GLUSTER_CLI_QUOTA" },
	{ GLUSTER_CLI_TOP_VOLUME, "GLUSTER_CLI_TOP_VOLUME" },
	{ GLUSTER_CLI_GETWD, "GLUSTER_CLI_GETWD" },
	{ GLUSTER_CLI_LOG_LEVEL, "GLUSTER_CLI_LOG_LEVEL" },
	{ GLUSTER_CLI_STATUS_VOLUME, "GLUSTER_CLI_STATUS_VOLUME" },
	{ GLUSTER_CLI_MOUNT, "GLUSTER_CLI_MOUNT" },
	{ GLUSTER_CLI_UMOUNT, "GLUSTER_CLI_UMOUNT" },
	{ GLUSTER_CLI_HEAL_VOLUME, "GLUSTER_CLI_HEAL_VOLUME" },
	{ GLUSTER_CLI_STATEDUMP_VOLUME, "GLUSTER_CLI_STATEDUMP_VOLUME" },
	{ GLUSTER_CLI_MAXVALUE, "GLUSTER_CLI_MAXVALUE" },
	{ 0, NULL }
};

/* procedures for GLUSTER_CBK_PROGRAM */
static const vsff gluster_cbk_proc[] = {
        { GF_CBK_NULL, "NULL", NULL, NULL },
        { GF_CBK_FETCHSPEC, "FETCHSPEC", NULL, NULL },
        { GF_CBK_INO_FLUSH, "INO_FLUSH", NULL, NULL },
	{ 0, NULL, NULL, NULL }
};
static const value_string gluster_cbk_proc_vals[] = {
        { GF_CBK_NULL, "NULL" },
        { GF_CBK_FETCHSPEC, "FETCHSPEC" },
        { GF_CBK_INO_FLUSH, "INO_FLUSH" },
	{ 0, NULL }
};

void
proto_register_gluster(void)
{
	/* Setup list of header fields  See Section 1.6.1 for details */
	static hf_register_info hf[] = {
		/* programs */
		{ &hf_gluster_mgmt_proc,
			{ "Gluster Management", "gluster.mgmt", FT_UINT32,
				BASE_DEC, VALS(gluster_mgmt_proc_vals), 0,
				NULL, HFILL }
		},
		{ &hf_gd_mgmt_proc,
			{ "Gluster Daemon Management", "glusterd.mgmt",
				FT_UINT32, BASE_DEC, VALS(gd_mgmt_proc_vals),
				0, NULL, HFILL }
		},
		{ &hf_gluster_cli_proc,
			{ "Gluster CLI", "gluster.cli", FT_UINT32, BASE_DEC,
				VALS(gluster_cli_proc_vals), 0, NULL, HFILL }
		},
		{ &hf_gluster_cbk_proc,
			{ "GlusterFS Callback", "gluster.cbk", FT_UINT32,
				BASE_DEC, VALS(gluster_cbk_proc_vals), 0, NULL,
				HFILL }
		},
		/* fields used by procedures */
		{ &hf_gluster_unknown_int,
			{ "Unknown Integer", "gluster.unknown.int", FT_UINT32,
				BASE_HEX, NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_gfid,
			{ "GFID", "gluster.gfid", FT_BYTES,
				BASE_NONE, NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_op,
			{ "Operation (FIXME?)", "gluster.op", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_op_ret,
			{ "Return value", "gluster.op_ret", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_op_errno,
			{ "Errno", "gluster.op_errno", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_op_errstr,
			{ "Error String", "gluster.op_errstr", FT_STRING,
				BASE_NONE, NULL, 0, NULL, HFILL }
		},

		{ &hf_gluster_uuid,
			{ "UUID", "gluster.uuid", FT_BYTES,
				BASE_NONE, NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_hostname,
			{ "Hostname", "gluster.hostname", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_port,
			{ "Port", "gluster.port", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_vols,
			{ "Volumes", "gluster.vols", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},


		{ &hf_gluster_dict,
			{ "Dict (unparsed)", "gluster.dict", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_dict_key,
			{ "Key", "gluster.dict.key", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_dict_value,
			{ "Value", "gluster.dict.value", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		}
	};

	/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_gluster,
		&ett_gluster_mgmt,
		&ett_gd_mgmt,
		&ett_gluster_cli,
		&ett_gluster_cbk,
		&ett_gluster_dict
	};

	/* Register the protocol name and description */
	proto_gluster = proto_register_protocol("Gluster", "Gluster",
								"gluster");
	proto_register_subtree_array(ett, array_length(ett));
	proto_register_field_array(proto_gluster, hf, array_length(hf));

	proto_gluster_mgmt = proto_register_protocol("Gluster Management",
					"Gluster Management", "gluster-mgmt");

	proto_gd_mgmt = proto_register_protocol("Gluster Daemon Management",
					"GlusterD Management", "gd-mgmt");

	proto_gluster_cli = proto_register_protocol("Gluster CLI",
					"Gluster CLI", "gluster-cli");

	proto_gluster_cbk = proto_register_protocol("GlusterFS Callback",
					"GlusterFS Callback", "gluster-cbk");
}


void
proto_reg_handoff_gluster(void)
{
	rpc_init_prog(proto_gluster_mgmt, GLUSTERD1_MGMT_PROGRAM,
							ett_gluster_mgmt);
	rpc_init_proc_table(GLUSTERD1_MGMT_PROGRAM, 1, gluster_mgmt_proc,
							hf_gluster_mgmt_proc);

	rpc_init_prog(proto_gd_mgmt, GD_MGMT_PROGRAM, ett_gd_mgmt);
	rpc_init_proc_table(GD_MGMT_PROGRAM, 1, gd_mgmt_proc, hf_gd_mgmt_proc);

	rpc_init_prog(proto_gluster_cli, GLUSTER_CLI_PROGRAM, ett_gluster_cli);
	rpc_init_proc_table(GLUSTER_CLI_PROGRAM, 1, gluster_cli_proc,
							hf_gluster_cli_proc);

	rpc_init_prog(proto_gluster_cbk, GLUSTER_CBK_PROGRAM, ett_gluster_cbk);
	rpc_init_proc_table(GLUSTER_CBK_PROGRAM, 1, gluster_cbk_proc,
							hf_gluster_cbk_proc);
}

