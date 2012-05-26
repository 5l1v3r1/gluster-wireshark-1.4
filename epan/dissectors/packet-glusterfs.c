/* packet-glusterfs.c
 * Routines for GlusterFS dissection
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
static gint proto_glusterfs = -1;

/* programs and procedures */
static gint hf_glusterfs_proc = -1;

/* fields used by multiple programs/procedures */
static gint hf_gluster_op_ret = -1;
static gint hf_gluster_op_errno = -1;
static gint hf_gluster_dict_key = -1;
static gint hf_gluster_dict_value = -1;

/* GlusterFS specific */
static gint hf_glusterfs_gfid = -1;
static gint hf_glusterfs_pargfid = -1;
static gint hf_glusterfs_oldgfid = -1;
static gint hf_glusterfs_newgfid = -1;
static gint hf_glusterfs_path = -1;
static gint hf_glusterfs_bname = -1;
static gint hf_glusterfs_dict = -1;
static gint hf_glusterfs_fd = -1;
static gint hf_glusterfs_offset = -1;
static gint hf_glusterfs_size = -1;
static gint hf_glusterfs_volume = -1;
static gint hf_glusterfs_cmd = -1;
static gint hf_glusterfs_type = -1;
static gint hf_glusterfs_entries = -1;
static gint hf_glusterfs_xflags = -1;
static gint hf_glusterfs_linkname = -1;
static gint hf_glusterfs_umask = -1;
static gint hf_glusterfs_mask = -1;
static gint hf_glusterfs_name = -1;
static gint hf_glusterfs_namelen = -1;

/* flags passed on to OPEN, CREATE etc.*/
static gint hf_glusterfs_flags = -1;
static gint hf_glusterfs_flags_rdonly = -1;
static gint hf_glusterfs_flags_wronly = -1;
static gint hf_glusterfs_flags_rdwr = -1;
static gint hf_glusterfs_flags_accmode = -1;
static gint hf_glusterfs_flags_append = -1;
static gint hf_glusterfs_flags_async = -1;
static gint hf_glusterfs_flags_cloexec = -1;
static gint hf_glusterfs_flags_creat = -1;
static gint hf_glusterfs_flags_direct = -1;
static gint hf_glusterfs_flags_directory = -1;
static gint hf_glusterfs_flags_excl = -1;
static gint hf_glusterfs_flags_largefile = -1;
static gint hf_glusterfs_flags_noatime = -1;
static gint hf_glusterfs_flags_noctty = -1;
static gint hf_glusterfs_flags_nofollow = -1;
static gint hf_glusterfs_flags_nonblock = -1;
static gint hf_glusterfs_flags_ndelay = -1;
static gint hf_glusterfs_flags_sync = -1;
static gint hf_glusterfs_flags_trunc = -1;

/* access modes  */
static gint hf_glusterfs_mode = -1;
static gint hf_glusterfs_mode_suid = -1;
static gint hf_glusterfs_mode_sgid = -1;
static gint hf_glusterfs_mode_svtx = -1;
static gint hf_glusterfs_mode_rusr = -1;
static gint hf_glusterfs_mode_wusr = -1;
static gint hf_glusterfs_mode_xusr = -1;
static gint hf_glusterfs_mode_rgrp = -1;
static gint hf_glusterfs_mode_wgrp = -1;
static gint hf_glusterfs_mode_xgrp = -1;
static gint hf_glusterfs_mode_roth = -1;
static gint hf_glusterfs_mode_woth = -1;
static gint hf_glusterfs_mode_xoth = -1;

/* dir-entry */
static gint hf_glusterfs_entry_ino = -1;
static gint hf_glusterfs_entry_off = -1;
static gint hf_glusterfs_entry_len = -1;
static gint hf_glusterfs_entry_type = -1;
static gint hf_glusterfs_entry_path = -1;

/* gf_iatt */
static gint hf_glusterfs_ia_ino = -1;
static gint hf_glusterfs_ia_dev = -1;
static gint hf_glusterfs_ia_mode = -1;
static gint hf_glusterfs_ia_nlink = -1;
static gint hf_glusterfs_ia_uid = -1;
static gint hf_glusterfs_ia_gid = -1;
static gint hf_glusterfs_ia_rdev = -1;
static gint hf_glusterfs_ia_size = -1;
static gint hf_glusterfs_ia_blksize = -1;
static gint hf_glusterfs_ia_blocks = -1;
static gint hf_glusterfs_ia_atime = -1;
static gint hf_glusterfs_ia_atime_nsec = -1;
static gint hf_glusterfs_ia_mtime = -1;
static gint hf_glusterfs_ia_mtime_nsec = -1;
static gint hf_glusterfs_ia_ctime = -1;
static gint hf_glusterfs_ia_ctime_nsec = -1;

/* gf_flock */
static gint hf_glusterfs_flock_type = -1;
static gint hf_glusterfs_flock_whence = -1;
static gint hf_glusterfs_flock_start = -1;
static gint hf_glusterfs_flock_len = -1;
static gint hf_glusterfs_flock_pid = -1;
static gint hf_glusterfs_flock_owner = -1;

/* statfs */
static gint hf_glusterfs_bsize = -1;
static gint hf_glusterfs_frsize = -1;
static gint hf_glusterfs_blocks = -1;
static gint hf_glusterfs_bfree = -1;
static gint hf_glusterfs_bavail = -1;
static gint hf_glusterfs_files = -1;
static gint hf_glusterfs_ffree = -1;
static gint hf_glusterfs_favail = -1;
static gint hf_glusterfs_id = -1;
static gint hf_glusterfs_flag = -1;
static gint hf_glusterfs_namemax = -1;

static gint hf_glusterfs_setattr_valid = -1;

/* Rename */
static gint hf_glusterfs_oldbname = -1;
static gint hf_glusterfs_newbname = -1;

/* for FSYNCDIR */
static gint hf_glusterfs_yncdir_data = -1;

/* for entrylk */
static gint hf_glusterfs_entrylk_namelen = -1;

/* Initialize the subtree pointers */
static gint ett_glusterfs = -1;
static gint ett_glusterfs_flags = -1;
static gint ett_glusterfs_mode = -1;
static gint ett_glusterfs_iatt = -1;
static gint ett_glusterfs_entry = -1;
static gint ett_glusterfs_flock = -1;
static gint ett_gluster_dict = -1;
static gint ett_gluster_dict_items = -1;

/* function for dissecting and adding a GFID to the tree
 *
 * Show as the by Gluster displayed string format
 * 00000000-0000-0000-0000-000000000001 (4-2-2-2-6 bytes).
 */
static int
glusterfs_item_append_gfid(proto_item *gfid_item, tvbuff_t *tvb, int offset)
{
	/* 4 bytes */
	proto_item_append_text(gfid_item, "%.2x", tvb_get_guint8(tvb, offset++));
	proto_item_append_text(gfid_item, "%.2x", tvb_get_guint8(tvb, offset++));
	proto_item_append_text(gfid_item, "%.2x", tvb_get_guint8(tvb, offset++));
	proto_item_append_text(gfid_item, "%.2x", tvb_get_guint8(tvb, offset++));
	/* 2 bytes */
	proto_item_append_text(gfid_item, "-%.2x", tvb_get_guint8(tvb, offset++));
	proto_item_append_text(gfid_item, "%.2x", tvb_get_guint8(tvb, offset++));
	/* 2 bytes */
	proto_item_append_text(gfid_item, "-%.2x", tvb_get_guint8(tvb, offset++));
	proto_item_append_text(gfid_item, "%.2x", tvb_get_guint8(tvb, offset++));
	/* 2 bytes */
	proto_item_append_text(gfid_item, "-%.2x", tvb_get_guint8(tvb, offset++));
	proto_item_append_text(gfid_item, "%.2x", tvb_get_guint8(tvb, offset++));
	/* 6 bytes */
	proto_item_append_text(gfid_item, "-%.2x", tvb_get_guint8(tvb, offset++));
	proto_item_append_text(gfid_item, "%.2x", tvb_get_guint8(tvb, offset++));
	proto_item_append_text(gfid_item, "%.2x", tvb_get_guint8(tvb, offset++));
	proto_item_append_text(gfid_item, "%.2x", tvb_get_guint8(tvb, offset++));
	proto_item_append_text(gfid_item, "%.2x", tvb_get_guint8(tvb, offset++));
	proto_item_append_text(gfid_item, "%.2x", tvb_get_guint8(tvb, offset++));

	return offset;
}

static int
glusterfs_rpc_dissect_gfid(proto_tree *tree, tvbuff_t *tvb, int hfindex, int offset)
{
	proto_item *gfid_item;

	if (tree) {
		header_field_info *hfinfo = proto_registrar_get_nth(hfindex);

		gfid_item = proto_tree_add_item(tree, hfindex, tvb, offset, 16, ENC_NA);
		PROTO_ITEM_SET_HIDDEN(gfid_item);

		gfid_item = proto_tree_add_text(tree, tvb, offset, 16, "%s: ", hfinfo->name);
		offset = glusterfs_item_append_gfid(gfid_item, tvb, offset);
	} else
		offset += 16;

	return offset;
}

static int
glusterfs_rpc_dissect_mode(proto_tree *tree, tvbuff_t *tvb, int hfindex, int offset)
{
	static const int *mode_bits[] = {
		&hf_glusterfs_mode_suid,
		&hf_glusterfs_mode_sgid,
		&hf_glusterfs_mode_svtx,
		&hf_glusterfs_mode_rusr,
		&hf_glusterfs_mode_wusr,
		&hf_glusterfs_mode_xusr,
		&hf_glusterfs_mode_rgrp,
		&hf_glusterfs_mode_wgrp,
		&hf_glusterfs_mode_xgrp,
		&hf_glusterfs_mode_roth,
		&hf_glusterfs_mode_woth,
		&hf_glusterfs_mode_xoth,
		NULL
	};

	if (tree)
		proto_tree_add_bitmask(tree, tvb, offset, hfindex, ett_glusterfs_mode, mode_bits, FALSE);

	offset += 4;
	return offset;
}

/*
 * from rpc/xdr/src/glusterfs3-xdr.c:xdr_gf_iatt()
 * FIXME: this may be inclomplete, different code-paths may require different
 * encoding/decoding.
 */
static int
glusterfs_rpc_dissect_gf_iatt(proto_tree *tree, tvbuff_t *tvb, int offset)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_ia_ino, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_ia_dev, offset);
	offset = glusterfs_rpc_dissect_mode(tree, tvb, hf_glusterfs_ia_mode, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_ia_nlink, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_ia_uid, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_ia_gid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_ia_rdev, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_ia_size, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_ia_blksize, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_ia_blocks, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_ia_atime, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_ia_atime_nsec, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_ia_mtime, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_ia_mtime_nsec, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_ia_ctime, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_ia_ctime_nsec, offset);

	return offset;
}

static int
glusterfs_rpc_dissect_gf_flock(proto_tree *tree, tvbuff_t *tvb, int offset)
{
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_flock_type, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_flock_whence, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_flock_start, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_flock_len, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_flock_pid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_flock_owner, offset);

	return offset;
}

static int
glusterfs_rpc_dissect_gf_2_flock(proto_tree *tree, tvbuff_t *tvb, int offset)
{
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_flock_type, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_flock_whence, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_flock_start, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_flock_len, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_flock_pid, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_rpc_dissect_flags(proto_tree *tree, tvbuff_t *tvb, int offset)
{
	gboolean rdonly, accmode;
	proto_item *flag_tree, *rdonly_item, *accmode_item;
	header_field_info *rdonly_hf, *accmode_hf;

	static const int *flag_bits[] = {
		&hf_glusterfs_flags_wronly,
		&hf_glusterfs_flags_rdwr,
		&hf_glusterfs_flags_creat,
		&hf_glusterfs_flags_excl,
		&hf_glusterfs_flags_noctty,
		&hf_glusterfs_flags_trunc,
		&hf_glusterfs_flags_append,
		&hf_glusterfs_flags_nonblock,
		&hf_glusterfs_flags_ndelay,
		&hf_glusterfs_flags_sync,
		&hf_glusterfs_flags_async,
		&hf_glusterfs_flags_direct,
		&hf_glusterfs_flags_largefile,
		&hf_glusterfs_flags_directory,
		&hf_glusterfs_flags_nofollow,
		&hf_glusterfs_flags_noatime,
		&hf_glusterfs_flags_cloexec,
		NULL
	};

	if (tree) {
		flag_tree = proto_tree_add_bitmask(tree, tvb, offset, hf_glusterfs_flags, ett_glusterfs_flags, flag_bits, FALSE);

		/* rdonly is TRUE only when no flags are set */
		rdonly = (tvb_get_ntohl(tvb, offset) == 0);
		rdonly_item = proto_tree_add_boolean(flag_tree, hf_glusterfs_flags_rdonly, tvb, offset, 4, rdonly);
		rdonly_hf = proto_registrar_get_nth(hf_glusterfs_flags_rdonly);
		/* show a static value of zero's, proto_tree_add_boolean() removes them */
		proto_item_set_text(rdonly_item, "0000 0000 0000 0000 0000 0000 0000 0000 = %s: %s",
			rdonly_hf->name, rdonly ? tfs_set_notset.true_string : tfs_set_notset.false_string);
		PROTO_ITEM_SET_GENERATED(rdonly_item);
		if (rdonly)
			proto_item_append_text(flag_tree, ", %s", rdonly_hf->name);

		/* hf_glusterfs_flags_accmode is TRUE if bits 0 and 1 are set */
		accmode_hf = proto_registrar_get_nth(hf_glusterfs_flags_accmode);
		accmode = ((tvb_get_ntohl(tvb, offset) & accmode_hf->bitmask) == accmode_hf->bitmask);
		accmode_item = proto_tree_add_boolean(flag_tree, hf_glusterfs_flags_accmode, tvb, offset, 4, accmode);
		PROTO_ITEM_SET_GENERATED(accmode_item);
		if (accmode)
			proto_item_append_text(flag_tree, ", %s", proto_registrar_get_nth(hf_glusterfs_flags_accmode)->name);
	}

	offset += 4;
	return offset;
}

static int
glusterfs_rpc_dissect_statfs(proto_tree *tree, tvbuff_t *tvb, int offset)
{
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_bsize, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_frsize, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_blocks, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_bfree, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_bavail, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_files, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_ffree, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_favail, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_id, offset);
	/* FIXME: hf_glusterfs_flag are flags, see 'man 2 statvfs' */
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_flag, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_namemax, offset);

	return offset;
}

/* function for dissecting and adding a gluster dict_t to the tree */
int
gluster_rpc_dissect_dict(proto_tree *tree, tvbuff_t *tvb, int hfindex, int offset)
{
	gchar *key, *value;
	gint items, i, len, roundup, value_len, key_len;

	proto_item *subtree_item;
	proto_tree *subtree;

	proto_item *dict_item;

	/* create a subtree for all the items in the dict */
	if (hfindex >= 0) {
		header_field_info *hfinfo = proto_registrar_get_nth(hfindex);
		subtree_item = proto_tree_add_text(tree, tvb, offset, -1, "%s", hfinfo->name);
	} else {
		subtree_item = proto_tree_add_text(tree, tvb, offset, -1, "<NAMELESS DICT STRUCTURE>");
	}

	subtree = proto_item_add_subtree(subtree_item, ett_gluster_dict);

	len = tvb_get_ntohl(tvb, offset);
	roundup = rpc_roundup(len) - len;
	proto_tree_add_text(subtree, tvb, offset, 4, "[Size: %d (%d bytes inc. RPC-roundup)]", len, rpc_roundup(len));
	offset += 4;

	if (len == 0)
		return offset;

	items = tvb_get_ntohl(tvb, offset);
	proto_tree_add_text(subtree, tvb, offset, 4, "[Items: %d]", items);
	offset += 4;

	for (i = 0; i < items; i++) {
		/* key_len is the length of the key without the terminating '\0' */
		/* key_len = tvb_get_ntohl(tvb, offset) + 1; // will be read later */
		offset += 4;
		value_len = tvb_get_ntohl(tvb, offset);
		offset += 4;

		/* read the key, '\0' terminated */
		key = tvb_get_stringz(tvb, offset, &key_len);
		if (tree)
			dict_item = proto_tree_add_text(subtree, tvb, offset, -1, "%s: ", key);
		offset += key_len;

		/* read the value, possibly '\0' terminated */
		value = tvb_get_string(tvb, offset, value_len);
		if (tree) {
			/* keys named "gfid-req" contain a GFID in hex */
			if (value_len == 16 && !strncmp("gfid-req", key, 8))
				glusterfs_item_append_gfid(dict_item, tvb, offset);
			else
				proto_item_append_text(dict_item, "%s", value);
		}
		offset += value_len;

		g_free(key);
		g_free(value);
	}

	if (roundup) {
		if (tree)
			proto_tree_add_text(subtree, tvb, offset, -1, "[RPC-roundup bytes: %d]", roundup);
		offset += roundup;
	}

	return offset;
}

int
gluster_dissect_common_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *errno_item;
	guint op_errno;

	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);

	if (tree) {
		op_errno = tvb_get_ntohl(tvb, offset);
		errno_item = proto_tree_add_int(tree, hf_gluster_op_errno, tvb,
					    offset, 4, op_errno);
		proto_item_append_text(errno_item, " (%s)",
							g_strerror(op_errno));
	}

	offset += 4;

	return offset;
}

/*
 *  372	  if (!xdr_int (xdrs, &objp->op_ret))
 *   373		  return FALSE;
 *    374	  if (!xdr_int (xdrs, &objp->op_errno))
 *     375		  return FALSE;
 *      376	  if (!xdr_gf_iatt (xdrs, &objp->preparent))
 *	377		  return FALSE;
 *	378	  if (!xdr_gf_iatt (xdrs, &objp->postparent))
 *
 */
static int
glusterfs_gfs3_op_unlink_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->preparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	return offset;
}

/*
 *  359	  if (!xdr_opaque (xdrs, objp->pargfid, 16))
 *   360		  return FALSE;
 *    361	  if (!xdr_string (xdrs, &objp->path, ~0))
 *     362		  return FALSE;
 *      363	  if (!xdr_string (xdrs, &objp->bname, ~0))
 *	364		  return FALSE;
 *
 */
static int
glusterfs_gfs3_op_unlink_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar* path = NULL;
	gchar* bname = NULL;
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_pargfid, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_path, offset, &path);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_bname, offset, &bname);
	return offset;
}

static int
glusterfs_gfs3_op_statfs_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	offset = glusterfs_rpc_dissect_statfs(tree, tvb, offset);
	return offset;
}

static int
glusterfs_gfs3_op_statfs_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *path = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_path, offset, &path);

	return offset;
}

static int
glusterfs_gfs3_op_flush_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	return offset;
}

static int
glusterfs_gfs3_op_setxattr_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *path = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = glusterfs_rpc_dissect_flags(tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_path, offset, &path);

	return offset;
}

static int
glusterfs_gfs3_op_opendir_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	return offset;
}

static int
glusterfs_gfs3_op_opendir_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *path = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_path, offset, &path);

	return offset;
}

/* rpc/xdr/src/glusterfs3-xdr.c:xdr_gfs3_create_rsp */
static int
glusterfs_gfs3_op_create_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->stat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "Stat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->preparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	return offset;
}

/* rpc/xdr/src/glusterfs3-xdr.c:xdr_gfs3_create_req */
static int
glusterfs_gfs3_op_create_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *path = NULL;
	gchar *bname = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_pargfid, offset);
	offset = glusterfs_rpc_dissect_flags(tree, tvb, offset);
	offset = glusterfs_rpc_dissect_mode(tree, tvb, hf_glusterfs_mode, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_path, offset, &path);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_bname, offset, &bname);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_op_lookup_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	/* FIXME: describe this better - gf_iatt (xdrs, &objp->stat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "Stat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_op_lookup_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *path = NULL;
	gchar *bname = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_pargfid, offset);
	offset = glusterfs_rpc_dissect_flags(tree, tvb, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_path, offset, &path);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_bname, offset, &bname);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_op_inodelk_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *flock_item;
	proto_tree *flock_tree;
	gchar* path = NULL;
	gchar* volume = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_cmd, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_type, offset);

	flock_item = proto_tree_add_text(tree, tvb, offset, -1, "flock");
	flock_tree = proto_item_add_subtree(flock_item, ett_glusterfs_flock);
	offset = glusterfs_rpc_dissect_gf_flock(flock_tree, tvb, offset);

	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_path, offset, &path);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_volume, offset, &volume);
	return offset;
}

static int
glusterfs_gfs3_op_readdirp_entry(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *entry_item, *iatt_item;
	proto_tree *entry_tree, *iatt_tree;
	gchar* path = NULL;

	entry_item = proto_tree_add_text(tree, tvb, offset, -1, "Entry");
	entry_tree = proto_item_add_subtree(entry_item, ett_glusterfs_entry);

	offset = dissect_rpc_uint64(tvb, entry_tree, hf_glusterfs_entry_ino, offset);
	offset = dissect_rpc_uint64(tvb, entry_tree, hf_glusterfs_entry_off, offset);
	offset = dissect_rpc_uint32(tvb, entry_tree, hf_glusterfs_entry_len, offset);
	offset = dissect_rpc_uint32(tvb, entry_tree, hf_glusterfs_entry_type, offset);
	offset = dissect_rpc_string(tvb, entry_tree, hf_glusterfs_entry_path, offset, &path);

	proto_item_append_text(entry_item, " Path:%s", path);

	iatt_item = proto_tree_add_text(entry_tree, tvb, offset, -1, "Stat");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	return offset;
}


/* details in xlators/storage/posix/src/posix.c:posix_fill_readdir() */
static int
glusterfs_gfs3_op_readdirp_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *errno_item;
	guint op_errno;

	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_entries, offset);

	if (tree) {
		op_errno = tvb_get_ntohl(tvb, offset);
		errno_item = proto_tree_add_int(tree, hf_gluster_op_errno, tvb,
					    offset, 4, op_errno);
		if (op_errno == 0)
			proto_item_append_text(errno_item,
					    " (More READDIRP replies follow)");
		else if (op_errno == 2 /* ENOENT */)
			proto_item_append_text(errno_item,
					    " (Last READDIRP reply)");
	}
	offset += 4;

	offset = dissect_rpc_list(tvb, pinfo, tree, offset,
					    glusterfs_gfs3_op_readdirp_entry);

	return offset;
}

static int
glusterfs_gfs3_op_readdirp_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_offset, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_size, offset);

	return offset;
}

static int
glusterfs_gfs3_op_setattr_reply(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);

	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "StatPre IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "StatPost IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	return offset;
}

static int
glusterfs_gfs3_op_setattr_call(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;
	gchar *path = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);

	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "stbuff IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: hf_glusterfs_setattr_valid is a flag
	 * see libglusterfs/src/xlator.h, #defines for GF_SET_ATTR_*
	 */
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_setattr_valid, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_path, offset, &path);

	return offset;
}

/*GlusterFS 3_3 fops */

static int
glusterfs_gfs3_3_op_stat_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);
	return offset;

}

static int
glusterfs_gfs3_3_op_stat_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);

	// FIXME: describe this better - gf_iatt (xdrs, &objp->stat
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "Stat Buf");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_mknod_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	/* FIXME: describe this better - gf_iatt (xdrs, &objp->stat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "Stat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	/* FIXME: describe this better - gf_iatt (xdrs, &objp->preparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);


	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_mknod_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{

	gchar *bname = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_pargfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_offset, offset);
	offset = glusterfs_rpc_dissect_mode(tree, tvb, hf_glusterfs_mode, offset);
	offset = glusterfs_rpc_dissect_mode(tree, tvb, hf_glusterfs_umask, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_bname, offset, &bname);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_mkdir_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	/* FIXME: describe this better - gf_iatt (xdrs, &objp->stat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "Stat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	/* FIXME: describe this better - gf_iatt (xdrs, &objp->preparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_mkdir_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *bname = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_pargfid, offset);
	offset = glusterfs_rpc_dissect_mode(tree, tvb, hf_glusterfs_mode, offset);
	offset = glusterfs_rpc_dissect_mode(tree, tvb, hf_glusterfs_umask, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_bname, offset, &bname);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_readlink_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	gchar* path = NULL;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "Buf IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_path, offset, &path);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);
	return offset;
}

static int
glusterfs_gfs3_3_op_readlink_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_size, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_unlink_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->preparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_unlink_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	guint xflags;
	gchar* bname = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_pargfid, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_bname, offset, &bname);
	xflags = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint_format(tree, hf_glusterfs_xflags, tvb, offset, 4, xflags, "Flags: 0%02o", xflags);
	offset += 4;
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_rmdir_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->preparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_rmdir_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar* bname = NULL;
	guint xflags;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_pargfid, offset);
	xflags = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint_format(tree, hf_glusterfs_xflags, tvb, offset, 4, xflags, "Flags: 0%02o", xflags);
	offset += 4;
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_bname, offset, &bname);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_symlink_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *bname = NULL;
	gchar *linkname = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_pargfid, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_bname, offset, &bname);
	offset = glusterfs_rpc_dissect_mode(tree, tvb, hf_glusterfs_umask, offset);

	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_linkname, offset, &linkname);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_symlink_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	/* FIXME: describe this better - gf_iatt (xdrs, &objp->stat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "Stat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	/* FIXME: describe this better - gf_iatt (xdrs, &objp->preparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_rename_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{

	gchar *oldbname = NULL;
	gchar *newbname = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_oldgfid, offset);
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_newgfid, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_oldbname, offset, &oldbname);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_newbname, offset, &newbname);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_rename_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{

	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);

	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "Stat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreOldParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostOldParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreNewParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostNewParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_link_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	 /* FIXME: describe this better - gf_iatt (xdrs, &objp->stat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "Stat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->preparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_link_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *newbname = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_oldgfid, offset);
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_newgfid, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_newbname, offset, &newbname);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_truncate_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->prestat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreStat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postStat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostStat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_truncate_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_offset, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_open_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);
	return offset;
}

static int
glusterfs_gfs3_3_op_open_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = glusterfs_rpc_dissect_flags(tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_read_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	/* FIXME: describe this better - gf_iatt (xdrs, &objp->stat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "Stat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_size, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_read_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_offset, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_size, offset);
	offset = glusterfs_rpc_dissect_flags(tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_write_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	/* FIXME: describe this better - gf_iatt (xdrs, &objp->prestat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreStat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	/* FIXME: describe this better - gf_iatt (xdrs, &objp->poststat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostStat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_write_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_offset, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_size, offset);
	offset = glusterfs_rpc_dissect_flags(tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_statfs_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	offset = glusterfs_rpc_dissect_statfs(tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_statfs_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_flush_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_fsync_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->prestat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreStat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postStat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostStat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}
static int
glusterfs_gfs3_3_op_fsync_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}
static int
glusterfs_gfs3_3_op_setxattr_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = glusterfs_rpc_dissect_flags(tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_getxattr_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar* name = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_namelen, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_name, offset, &name);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}


static int
glusterfs_gfs3_3_op_getxattr_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_removexattr_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar* name = NULL;
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_name, offset, &name);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_fsyncdir_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_yncdir_data, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_opendir_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_opendir_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_create_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->stat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "Stat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->preparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}


static int
glusterfs_gfs3_3_op_create_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *bname = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_pargfid, offset);
	offset = glusterfs_rpc_dissect_flags(tree, tvb, offset);
	offset = glusterfs_rpc_dissect_mode(tree, tvb, hf_glusterfs_mode, offset);
	offset = glusterfs_rpc_dissect_mode(tree, tvb, hf_glusterfs_umask, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_bname, offset, &bname);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_ftruncate_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_offset, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}


static int
glusterfs_gfs3_3_op_ftruncate_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->prestat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreStat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postStat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostStat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_fstat_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_fstat_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->prestat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "Stat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_lk_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *flock_item;
	proto_tree *flock_tree;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_cmd, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_type, offset);

	flock_item = proto_tree_add_text(tree, tvb, offset, -1, "flock");
	flock_tree = proto_item_add_subtree(flock_item, ett_glusterfs_flock);
	offset = glusterfs_rpc_dissect_gf_2_flock(flock_tree, tvb, offset);

	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_lk_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *flock_item;
	proto_tree *flock_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	flock_item = proto_tree_add_text(tree, tvb, offset, -1, "flock");
	flock_tree = proto_item_add_subtree(flock_item, ett_glusterfs_flock);
	offset = glusterfs_rpc_dissect_gf_2_flock(flock_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_access_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_mask, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);
	return offset;
}

static int
glusterfs_gfs3_3_op_lookup_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	/* FIXME: describe this better - gf_iatt (xdrs, &objp->stat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "Stat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_lookup_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *bname = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_pargfid, offset);
	offset = glusterfs_rpc_dissect_flags(tree, tvb, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_bname, offset, &bname);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_readdir_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_offset, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_size, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_inodelk_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *flock_item;
	proto_tree *flock_tree;
	gchar* volume = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_cmd, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_type, offset);

	flock_item = proto_tree_add_text(tree, tvb, offset, -1, "flock");
	flock_tree = proto_item_add_subtree(flock_item, ett_glusterfs_flock);
	offset = glusterfs_rpc_dissect_gf_2_flock(flock_tree, tvb, offset);

	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_volume, offset, &volume);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_finodelk_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *flock_item;
	proto_tree *flock_tree;
	gchar* volume = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);

	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_cmd, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_type, offset);

	flock_item = proto_tree_add_text(tree, tvb, offset, -1, "flock");
	flock_tree = proto_item_add_subtree(flock_item, ett_glusterfs_flock);
	offset = glusterfs_rpc_dissect_gf_2_flock(flock_tree, tvb, offset);

	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_volume, offset, &volume);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_entrylk_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar* volume = NULL;
	gchar* name = NULL;
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_cmd, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_type, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_entrylk_namelen, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_name, offset, &name);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_volume, offset, &volume);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_fentrylk_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar* volume = NULL;
	gchar* name = NULL;
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_cmd, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_type, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_entrylk_namelen, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_name, offset, &name);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_volume, offset, &volume);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_xattrop_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_xattrop_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = glusterfs_rpc_dissect_flags(tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_fxattrop_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = glusterfs_rpc_dissect_flags(tree, tvb, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_fgetxattr_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar* name = NULL;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_namelen, offset);
	offset = dissect_rpc_string(tvb, tree, hf_glusterfs_name, offset, &name);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
gluter_gfs3_3_op_fsetxattr_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = glusterfs_rpc_dissect_flags(tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_setattr_reply(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);

	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "StatPre IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "StatPost IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
 	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_setattr_call(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);

	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "stbuff IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_glusterfs_iatt);
	offset = glusterfs_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: hf_glusterfs_setattr_valid is a flag
	 * see libglusterfs/src/xlator.h, #defines for GF_SET_ATTR_*
	 */
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_setattr_valid, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_readdirp_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *errno_item;
        guint op_errno;

	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_entries, offset);

        if (tree) {
		op_errno = tvb_get_ntohl(tvb, offset);
		errno_item = proto_tree_add_int(tree, hf_gluster_op_errno, tvb,
					    offset, 4, op_errno);
		if (op_errno == 0)
			proto_item_append_text(errno_item,
					    " (More READDIRP replies follow)");
		else if (op_errno == 2 /* ENOENT */)
			proto_item_append_text(errno_item,
					    " (Last READDIRP reply)");
	}
	offset += 4;

	offset = dissect_rpc_list(tvb, pinfo, tree, offset,
					    glusterfs_gfs3_op_readdirp_entry);

        return offset;
}

static int
glusterfs_gfs3_3_op_readdirp_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_offset, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_glusterfs_size, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_release_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

static int
glusterfs_gfs3_3_op_releasedir_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = glusterfs_rpc_dissect_gfid(tree, tvb, hf_glusterfs_gfid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_glusterfs_fd, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

/* This function is for common replay. RELEASE , RELEASEDIR and some other function use this method */

static int
glusterfs_gfs3_3_op_common_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = gluster_dissect_common_reply(tvb, offset, pinfo, tree);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_glusterfs_dict, offset);

	return offset;
}

/*
 * GLUSTER3_1_FOP_PROGRAM
 * - xlators/protocol/client/src/client3_1-fops.c
 * - xlators/protocol/server/src/server3_1-fops.c
 */
static const vsff glusterfs3_1_fop_proc[] = {
	{ GFS3_OP_NULL, "NULL", NULL, NULL },
	{ GFS3_OP_STAT, "STAT", NULL, NULL },
	{ GFS3_OP_READLINK, "READLINK", NULL, NULL },
	{ GFS3_OP_MKNOD, "MKNOD", NULL, NULL },
	{ GFS3_OP_MKDIR, "MKDIR", NULL, NULL },
	{
		GFS3_OP_UNLINK, "UNLINK",
		glusterfs_gfs3_op_unlink_call, glusterfs_gfs3_op_unlink_reply
	},
	{ GFS3_OP_RMDIR, "RMDIR", NULL, NULL },
	{ GFS3_OP_SYMLINK, "SYMLINK", NULL, NULL },
	{ GFS3_OP_RENAME, "RENAME", NULL, NULL },
	{ GFS3_OP_LINK, "LINK", NULL, NULL },
	{ GFS3_OP_TRUNCATE, "TRUNCATE", NULL, NULL },
	{ GFS3_OP_OPEN, "OPEN", NULL, NULL },
	{ GFS3_OP_READ, "READ", NULL, NULL },
	{ GFS3_OP_WRITE, "WRITE", NULL, NULL },
	{
		GFS3_OP_STATFS, "STATFS",
		glusterfs_gfs3_op_statfs_call, glusterfs_gfs3_op_statfs_reply
	},
	{
		GFS3_OP_FLUSH, "FLUSH",
		glusterfs_gfs3_op_flush_call, gluster_dissect_common_reply
	},
	{ GFS3_OP_FSYNC, "FSYNC", NULL, NULL },
	{
		GFS3_OP_SETXATTR, "SETXATTR",
		glusterfs_gfs3_op_setxattr_call, gluster_dissect_common_reply
	},
	{ GFS3_OP_GETXATTR, "GETXATTR", NULL, NULL },
	{ GFS3_OP_REMOVEXATTR, "REMOVEXATTR", NULL, NULL },
	{
		GFS3_OP_OPENDIR, "OPENDIR",
		glusterfs_gfs3_op_opendir_call, glusterfs_gfs3_op_opendir_reply
	},
	{ GFS3_OP_FSYNCDIR, "FSYNCDIR", NULL, NULL },
	{ GFS3_OP_ACCESS, "ACCESS", NULL, NULL },
	{
		GFS3_OP_CREATE, "CREATE",
		glusterfs_gfs3_op_create_call, glusterfs_gfs3_op_create_reply
	},
	{ GFS3_OP_FTRUNCATE, "FTRUNCATE", NULL, NULL },
	{ GFS3_OP_FSTAT, "FSTAT", NULL, NULL },
	{ GFS3_OP_LK, "LK", NULL, NULL },
	{
		GFS3_OP_LOOKUP, "LOOKUP",
		glusterfs_gfs3_op_lookup_call, glusterfs_gfs3_op_lookup_reply
	},
	{ GFS3_OP_READDIR, "READDIR", NULL, NULL },
	{
		GFS3_OP_INODELK, "INODELK",
		glusterfs_gfs3_op_inodelk_call, gluster_dissect_common_reply
	},
	{ GFS3_OP_FINODELK, "FINODELK", NULL, NULL },
	{ GFS3_OP_ENTRYLK, "ENTRYLK", NULL, NULL },
	{ GFS3_OP_FENTRYLK, "FENTRYLK", NULL, NULL },
	{ GFS3_OP_XATTROP, "XATTROP", NULL, NULL },
	{ GFS3_OP_FXATTROP, "FXATTROP", NULL, NULL },
	{ GFS3_OP_FGETXATTR, "FGETXATTR", NULL, NULL },
	{ GFS3_OP_FSETXATTR, "FSETXATTR", NULL, NULL },
	{ GFS3_OP_RCHECKSUM, "RCHECKSUM", NULL, NULL },
	{
		GFS3_OP_SETATTR, "SETATTR",
		glusterfs_gfs3_op_setattr_call, glusterfs_gfs3_op_setattr_reply
	},
	{
		GFS3_OP_FSETATTR, "FSETATTR",
		/* SETATTR and SETFATTS calls and reply are encoded the same */
		glusterfs_gfs3_op_setattr_call, glusterfs_gfs3_op_setattr_reply
	},
	{
		GFS3_OP_READDIRP, "READDIRP",
		glusterfs_gfs3_op_readdirp_call, glusterfs_gfs3_op_readdirp_reply
	},
	{ GFS3_OP_RELEASE, "RELEASE", NULL, NULL },
	{ GFS3_OP_RELEASEDIR, "RELEASEDIR", NULL, NULL },
	{ 0, NULL, NULL, NULL }
};


/*
 * GLUSTER3_1_FOP_PROGRAM for 3_3
 * - xlators/protocol/client/src/client3_1-fops.c
 * - xlators/protocol/server/src/server3_1-fops.c
 */
static const vsff glusterfs3_3_fop_proc[] = {
	{ GFS3_OP_NULL, "NULL", NULL, NULL },
	{
	 	GFS3_OP_STAT, "STAT",
		glusterfs_gfs3_3_op_stat_call, glusterfs_gfs3_3_op_stat_reply
	},
	{
		GFS3_OP_READLINK, "READLINK",
		glusterfs_gfs3_3_op_readlink_call, glusterfs_gfs3_3_op_readlink_reply
	},
	{
		GFS3_OP_MKNOD, "MKNOD",
		glusterfs_gfs3_3_op_mknod_call, glusterfs_gfs3_3_op_mknod_reply
	},
	{
		GFS3_OP_MKDIR, "MKDIR",
		glusterfs_gfs3_3_op_mkdir_call, glusterfs_gfs3_3_op_mkdir_reply
	},
	{
		GFS3_OP_UNLINK, "UNLINK",
		glusterfs_gfs3_3_op_unlink_call, glusterfs_gfs3_3_op_unlink_reply
	},
	{
		GFS3_OP_RMDIR, "RMDIR",
		glusterfs_gfs3_3_op_rmdir_call, glusterfs_gfs3_3_op_rmdir_reply
	},
	{ 	GFS3_OP_SYMLINK, "SYMLINK",
		glusterfs_gfs3_3_op_symlink_call, glusterfs_gfs3_3_op_symlink_reply
	},
	{
		GFS3_OP_RENAME, "RENAME",
		glusterfs_gfs3_3_op_rename_call, glusterfs_gfs3_3_op_rename_reply
	},
	{
		GFS3_OP_LINK, "LINK",
		glusterfs_gfs3_3_op_link_call, glusterfs_gfs3_3_op_link_reply
	},
	{
		GFS3_OP_TRUNCATE, "TRUNCATE",
		glusterfs_gfs3_3_op_truncate_call, glusterfs_gfs3_3_op_truncate_reply
	},
	{
		GFS3_OP_OPEN, "OPEN",
		glusterfs_gfs3_3_op_open_call, glusterfs_gfs3_3_op_open_reply
	},
	{
		GFS3_OP_READ, "READ",
		glusterfs_gfs3_3_op_read_call, glusterfs_gfs3_3_op_read_reply
	},
	{
		GFS3_OP_WRITE, "WRITE",
		glusterfs_gfs3_3_op_write_call, glusterfs_gfs3_3_op_write_reply
	},
	{
		GFS3_OP_STATFS, "STATFS",
		glusterfs_gfs3_3_op_statfs_call, glusterfs_gfs3_3_op_statfs_reply
	},
	{
		GFS3_OP_FLUSH, "FLUSH",
		glusterfs_gfs3_3_op_flush_call, glusterfs_gfs3_3_op_common_reply
	},
	{
		GFS3_OP_FSYNC, "FSYNC",
		glusterfs_gfs3_3_op_fsync_call, glusterfs_gfs3_3_op_fsync_reply
	},
	{
		GFS3_OP_SETXATTR, "SETXATTR",
		glusterfs_gfs3_3_op_setxattr_call, glusterfs_gfs3_3_op_common_reply
	},
	{
		GFS3_OP_GETXATTR, "GETXATTR",
		glusterfs_gfs3_3_op_getxattr_call, glusterfs_gfs3_3_op_getxattr_reply
	},
	{
		GFS3_OP_REMOVEXATTR, "REMOVEXATTR",
		glusterfs_gfs3_3_op_removexattr_call, glusterfs_gfs3_3_op_common_reply
	},
	{
		GFS3_OP_OPENDIR, "OPENDIR",
		glusterfs_gfs3_3_op_opendir_call, glusterfs_gfs3_3_op_opendir_reply
	},
	{
		GFS3_OP_FSYNCDIR, "FSYNCDIR",
		glusterfs_gfs3_3_op_fsyncdir_call, glusterfs_gfs3_3_op_common_reply
	},
	{
		GFS3_OP_ACCESS, "ACCESS",
		glusterfs_gfs3_3_op_access_call, glusterfs_gfs3_3_op_common_reply
	},
	{
		GFS3_OP_CREATE, "CREATE",
		glusterfs_gfs3_3_op_create_call, glusterfs_gfs3_3_op_create_reply
	},
	{
		GFS3_OP_FTRUNCATE, "FTRUNCATE",
		glusterfs_gfs3_3_op_ftruncate_call, glusterfs_gfs3_3_op_ftruncate_reply
	},
	{
		GFS3_OP_FSTAT, "FSTAT",
		glusterfs_gfs3_3_op_fstat_call, glusterfs_gfs3_3_op_fstat_reply
	},
	{
		GFS3_OP_LK, "LK",
		glusterfs_gfs3_3_op_lk_call, glusterfs_gfs3_3_op_lk_reply
	},
	{
		GFS3_OP_LOOKUP, "LOOKUP",
		glusterfs_gfs3_3_op_lookup_call, glusterfs_gfs3_3_op_lookup_reply
	},
	{
		GFS3_OP_READDIR, "READDIR",
		glusterfs_gfs3_3_op_readdir_call, glusterfs_gfs3_3_op_common_reply
	},
	{
		GFS3_OP_INODELK, "INODELK",
		glusterfs_gfs3_3_op_inodelk_call, glusterfs_gfs3_3_op_common_reply
	},
	{
		GFS3_OP_FINODELK, "FINODELK",
		glusterfs_gfs3_3_op_finodelk_call, glusterfs_gfs3_3_op_common_reply
	},
	{
		GFS3_OP_ENTRYLK, "ENTRYLK",
		glusterfs_gfs3_3_op_entrylk_call, glusterfs_gfs3_3_op_common_reply
	},
	{
		GFS3_OP_FENTRYLK, "FENTRYLK",
		glusterfs_gfs3_3_op_fentrylk_call, glusterfs_gfs3_3_op_common_reply
	},
	{
		GFS3_OP_XATTROP, "XATTROP",
		glusterfs_gfs3_3_op_xattrop_call, glusterfs_gfs3_3_op_xattrop_reply
	},
	/*xattrop and fxattrop replay both are same */
	{
		GFS3_OP_FXATTROP, "FXATTROP",
		glusterfs_gfs3_3_op_fxattrop_call, glusterfs_gfs3_3_op_xattrop_reply
	},
	{
		GFS3_OP_FGETXATTR, "FGETXATTR",
		glusterfs_gfs3_3_op_fgetxattr_call, glusterfs_gfs3_3_op_xattrop_reply
	},
	{
		GFS3_OP_FSETXATTR, "FSETXATTR",
		gluter_gfs3_3_op_fsetxattr_call, glusterfs_gfs3_3_op_common_reply
	},
	{ GFS3_OP_RCHECKSUM, "RCHECKSUM", NULL, NULL },
	{
		GFS3_OP_SETATTR, "SETATTR",
		glusterfs_gfs3_3_op_setattr_call, glusterfs_gfs3_3_op_setattr_reply
	},
	{
		GFS3_OP_FSETATTR, "FSETATTR",
		/* SETATTR and SETFATTS calls and reply are encoded the same */
		glusterfs_gfs3_3_op_setattr_call, glusterfs_gfs3_3_op_setattr_reply
	},
	{
		GFS3_OP_READDIRP, "READDIRP",
		glusterfs_gfs3_3_op_readdirp_call, glusterfs_gfs3_3_op_readdirp_reply
	},
	{
		GFS3_OP_RELEASE, "RELEASE",
		glusterfs_gfs3_3_op_release_call, glusterfs_gfs3_3_op_common_reply
	},
	{
		GFS3_OP_RELEASEDIR, "RELEASEDIR",
 		glusterfs_gfs3_3_op_releasedir_call, glusterfs_gfs3_3_op_common_reply
	},
	{ 0, NULL, NULL, NULL }
};


static const value_string glusterfs3_1_fop_proc_vals[] = {
	{ GFS3_OP_NULL, "NULL" },
	{ GFS3_OP_STAT, "STAT" },
	{ GFS3_OP_READLINK, "READLINK" },
	{ GFS3_OP_MKNOD, "MKNOD" },
	{ GFS3_OP_MKDIR, "MKDIR" },
	{ GFS3_OP_UNLINK, "UNLINK" },
	{ GFS3_OP_RMDIR, "RMDIR" },
	{ GFS3_OP_SYMLINK, "SYMLINK" },
	{ GFS3_OP_RENAME, "RENAME" },
	{ GFS3_OP_LINK, "LINK" },
	{ GFS3_OP_TRUNCATE, "TRUNCATE" },
	{ GFS3_OP_OPEN, "OPEN" },
	{ GFS3_OP_READ, "READ" },
	{ GFS3_OP_WRITE, "WRITE" },
	{ GFS3_OP_STATFS, "STATFS" },
	{ GFS3_OP_FLUSH, "FLUSH" },
	{ GFS3_OP_FSYNC, "FSYNC" },
	{ GFS3_OP_SETXATTR, "SETXATTR" },
	{ GFS3_OP_GETXATTR, "GETXATTR" },
	{ GFS3_OP_REMOVEXATTR, "REMOVEXATTR" },
	{ GFS3_OP_OPENDIR, "OPENDIR" },
	{ GFS3_OP_FSYNCDIR, "FSYNCDIR" },
	{ GFS3_OP_ACCESS, "ACCESS" },
	{ GFS3_OP_CREATE, "CREATE" },
	{ GFS3_OP_FTRUNCATE, "FTRUNCATE" },
	{ GFS3_OP_FSTAT, "FSTAT" },
	{ GFS3_OP_LK, "LK" },
	{ GFS3_OP_LOOKUP, "LOOKUP" },
	{ GFS3_OP_READDIR, "READDIR" },
	{ GFS3_OP_INODELK, "INODELK" },
	{ GFS3_OP_FINODELK, "FINODELK" },
	{ GFS3_OP_ENTRYLK, "ENTRYLK" },
	{ GFS3_OP_FENTRYLK, "FENTRYLK" },
	{ GFS3_OP_XATTROP, "XATTROP" },
	{ GFS3_OP_FXATTROP, "FXATTROP" },
	{ GFS3_OP_FGETXATTR, "FGETXATTR" },
	{ GFS3_OP_FSETXATTR, "FSETXATTR" },
	{ GFS3_OP_RCHECKSUM, "RCHECKSUM" },
	{ GFS3_OP_SETATTR, "SETATTR" },
	{ GFS3_OP_FSETATTR, "FSETATTR" },
	{ GFS3_OP_READDIRP, "READDIRP" },
	{ GFS3_OP_RELEASE, "RELEASE" },
	{ GFS3_OP_RELEASEDIR, "RELEASEDIR" },
	{ 0, NULL }
};

/* dir-entry types */
static const value_string glusterfs_entry_type_names[] = {
	{ DT_UNKNOWN, "DT_UNKNOWN" },
	{ DT_FIFO, "DT_FIFO" },
	{ DT_CHR, "DT_CHR" },
	{ DT_DIR, "DT_DIR" },
	{ DT_BLK, "DT_BLK" },
	{ DT_REG, "DT_REG" },
	{ DT_LNK, "DT_LNK" },
	{ DT_SOCK, "DT_SOCK" },
	{ DT_WHT, "DT_WHT" },
	{ 0, NULL }
};

/* Normal locking commands */
static const value_string glusterfs_lk_cmd_names[] = {
	{ GF_LK_GETLK, "GF_LK_GETLK" },
	{ GF_LK_SETLK, "GF_LK_SETLK" },
	{ GF_LK_SETLKW, "GF_LK_SETLKW" },
	{ GF_LK_RESLK_LCK, "GF_LK_RESLK_LCK" },
	{ GF_LK_RESLK_LCKW, "GF_LK_RESLK_LCKW" },
	{ GF_LK_RESLK_UNLCK, "GF_LK_RESLK_UNLCK" },
	{ GF_LK_GETLK_FD, "GF_LK_GETLK_FD" },
	{ 0, NULL }
};

/* Different lock types */
static const value_string glusterfs_lk_type_names[] = {
	{ GF_LK_F_RDLCK, "GF_LK_F_RDLCK" },
	{ GF_LK_F_WRLCK, "GF_LK_F_WRLCK" },
	{ GF_LK_F_UNLCK, "GF_LK_F_UNLCK" },
	{ GF_LK_EOL, "GF_LK_EOL" },
	{ 0, NULL }
};

void
proto_register_glusterfs(void)
{
	/* Setup list of header fields  See Section 1.6.1 for details */
	static hf_register_info hf[] = {
		/* programs */
		{ &hf_glusterfs_proc,
			{ "GlusterFS", "glusterfs", FT_UINT32, 	BASE_DEC,
				VALS(glusterfs3_1_fop_proc_vals), 0, NULL, HFILL }
		},
		/* fields used by multiple programs/procedures */
		{ &hf_gluster_op_ret,
			{ "Return value", "gluster.op_ret", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_op_errno,
			{ "Errno", "gluster.op_errno", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_dict_key,
			{ "Key", "gluster.dict.key", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_dict_value,
			{ "Value", "gluster.dict.value", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		/* GlusterFS specific */
		{ &hf_glusterfs_gfid,
			{ "GFID", "glusterfs.gfid", FT_BYTES,
				BASE_NONE, NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_pargfid,
			{ "PARGFID (FIXME?)", "glusterfs.pargfid", FT_BYTES,
				BASE_NONE, NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_oldgfid,
			{ "Old GFID", "glusterfs.oldgfid", FT_BYTES,
				BASE_NONE, NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_newgfid,
			{ "New GFID", "glusterfs.newgfid", FT_BYTES,
				BASE_NONE, NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_path,
			{ "Path", "glusterfs.path", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_bname,
			{ "Basename", "glusterfs.bname", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_dict,
			{ "Dict", "glusterfs.dict", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_fd,
			{ "File Descriptor", "glusterfs.fd", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_offset,
			{ "Offset", "glusterfs.offset", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_size,
			{ "Size", "glusterfs.size", FT_UINT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_type,
			{ "Type", "glusterfs.type", FT_INT32, BASE_DEC,
				VALS(glusterfs_lk_type_names), 0, NULL, HFILL }
		},
		{ &hf_glusterfs_cmd,
			{ "Command", "glusterfs.cmd", FT_INT32, BASE_DEC,
				VALS(glusterfs_lk_cmd_names), 0, NULL, HFILL }
		},
		{ &hf_glusterfs_volume,
			{ "Volume", "glusterfs.volume", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_namelen,
			{ "Name Lenth", "glusterfs.namelen", FT_UINT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_linkname,
			{ "Linkname", "glusterfs.linkname", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_umask,
			{ "Umask", "glusterfs.umask", FT_UINT32, BASE_OCT,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_mask,
			{ "Mask", "glusterfs.mask", FT_UINT32, BASE_OCT,
				NULL, 0, NULL, HFILL }
		},

		{ &hf_glusterfs_entries, /* READDIRP returned <x> entries */
			{ "Entries returned", "glusterfs.entries", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		/* Flags passed on to OPEN, CREATE etc, based on */
		{ &hf_glusterfs_flags,
			{ "Flags", "glusterfs.flags", FT_UINT32, BASE_OCT,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_rdonly,
			{ "O_RDONLY", "glusterfs.flags.rdonly", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00000000, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_wronly,
			{ "O_WRONLY", "glusterfs.flags.wronly", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00000001, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_rdwr,
			{ "O_RDWR", "glusterfs.flags.rdwr", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00000002, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_accmode,
			{ "O_ACCMODE", "glusterfs.flags.accmode", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00000003, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_append,
			{ "O_APPEND", "glusterfs.flags.append", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00002000, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_async,
			{ "O_ASYNC", "glusterfs.flags.async", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00020000, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_cloexec,
			{ "O_CLOEXEC", "glusterfs.flags.cloexec", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 02000000, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_creat,
			{ "O_CREAT", "glusterfs.flags.creat", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00000100, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_direct,
			{ "O_DIRECT", "glusterfs.flags.direct", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00040000, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_directory,
			{ "O_DIRECTORY", "glusterfs.flags.directory", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00200000, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_excl,
			{ "O_EXCL", "glusterfs.flags.excl", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00000200, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_largefile,
			{ "O_LARGEFILE", "glusterfs.flags.largefile", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00100000, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_noatime,
			{ "O_NOATIME", "glusterfs.flags.noatime", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 01000000, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_noctty,
			{ "O_NOCTTY", "glusterfs.flags.noctty", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00000400, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_nofollow,
			{ "O_NOFOLLOW", "glusterfs.flags.nofollow", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00400000, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_nonblock,
			{ "O_NONBLOCK", "glusterfs.flags.nonblock", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00004000, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_ndelay,
			{ "O_NDELAY", "glusterfs.flags.ndelay", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00004000, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_sync,
			{ "O_SYNC", "glusterfs.flags.sync", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00010000, NULL, HFILL }
		},
		{ &hf_glusterfs_flags_trunc,
			{ "O_TRUNC", "glusterfs.flags.trunc", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), 00001000, NULL, HFILL }
		},
		/* access modes */
		{ &hf_glusterfs_mode,
			{ "Mode", "glusterfs.mode", FT_UINT32, BASE_OCT,
				NULL, 0, "Access Permissions", HFILL }
		},
		{ &hf_glusterfs_mode_suid,
			{ "S_ISUID", "glusterfs.mode.s_isuid", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), (04000), "set-user-ID", HFILL }
		},
		{ &hf_glusterfs_mode_sgid,
			{ "S_ISGID", "glusterfs.mode.s_isgid", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), (02000), "set-group-ID", HFILL }
		},
		{ &hf_glusterfs_mode_svtx,
			{ "S_ISVTX", "glusterfs.mode.s_isvtx", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), (01000), "sticky bit", HFILL }
		},
		{ &hf_glusterfs_mode_rusr,
			{ "S_IRUSR", "glusterfs.mode.s_irusr", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), (00400), "read by owner", HFILL }
		},
		{ &hf_glusterfs_mode_wusr,
			{ "S_IWUSR", "glusterfs.mode.s_iwusr", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), (00200), "write by owner", HFILL }
		},
		{ &hf_glusterfs_mode_xusr,
			{ "S_IXUSR", "glusterfs.mode.s_ixusr", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), (00100), "execute/search by owner", HFILL }
		},
		{ &hf_glusterfs_mode_rgrp,
			{ "S_IRGRP", "glusterfs.mode.s_irgrp", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), (00040), "read by group", HFILL }
		},
		{ &hf_glusterfs_mode_wgrp,
			{ "S_IWGRP", "glusterfs.mode.s_iwgrp", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), (00020), "write by group", HFILL }
		},
		{ &hf_glusterfs_mode_xgrp,
			{ "S_IXGRP", "glusterfs.mode.s_ixgrp", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), (00010), "execute/search by group", HFILL }
		},
		{ &hf_glusterfs_mode_roth,
			{ "S_IROTH", "glusterfs.mode.s_iroth", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), (00004), "read by others", HFILL }
		},
		{ &hf_glusterfs_mode_woth,
			{ "S_IWOTH", "glusterfs.mode.s_iwoth", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), (00002), "write by others", HFILL }
		},
		{ &hf_glusterfs_mode_xoth,
			{ "S_IXOTH", "glusterfs.mode.s_ixoth", FT_BOOLEAN, 32,
				TFS(&tfs_set_notset), (00001), "execute/search by others", HFILL }
		},
		/* the dir-entry structure */
		{ &hf_glusterfs_entry_ino,
			{ "Inode", "glusterfs.entry.ino", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_entry_off, /* like telldir() */
			{ "Offset", "glusterfs.entry.d_off", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_entry_len, /* length of the path string */
			{ "Path length", "glusterfs.entry.len", FT_UINT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_entry_type,
			{ "Type", "glusterfs.entry.d_type", FT_UINT32, BASE_DEC,
				VALS(glusterfs_entry_type_names), 0, NULL, HFILL }
		},
		{ &hf_glusterfs_entry_path,
			{ "Path", "glusterfs.entry.path", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		/* the IATT structure */
		{ &hf_glusterfs_ia_ino,
			{ "ia_ino", "glusterfs.ia_ino", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ia_dev,
			{ "ia_dev", "glusterfs.ia_dev", FT_UINT64, BASE_HEX,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ia_mode,
			{ "ia_dev", "glusterfs.ia_mode", FT_UINT32, BASE_OCT,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ia_nlink,
			{ "ia_nlink", "glusterfs.ia_nlink", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ia_uid,
			{ "ia_uid", "glusterfs.ia_uid", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ia_gid,
			{ "ia_gid", "glusterfs.ia_gid", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ia_rdev,
			{ "ia_rdev", "glusterfs.ia_rdev", FT_UINT64, BASE_HEX,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ia_size,
			{ "ia_size", "glusterfs.ia_size", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ia_blksize,
			{ "ia_blksize", "glusterfs.ia_blksize", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ia_blocks,
			{ "ia_blocks", "glusterfs.ia_blocks", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ia_atime,
			{ "ia_time", "glusterfs.is_time", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ia_atime_nsec,
			{ "ia_atime_nsec", "glusterfs.ia_atime_nsec", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ia_mtime,
			{ "ia_mtime", "glusterfs.is_mtime", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ia_mtime_nsec,
			{ "ia_mtime_msec", "glusterfs.is_mtime_nsec", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ia_ctime,
			{ "ia_ctime", "glusterfs.ia_ctime", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ia_ctime_nsec,
			{ "ia_ctime_nsec", "glusterfs.ia_ctime_nsec", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},

		/* gf_flock */
		{ &hf_glusterfs_flock_type,
			{ "ia_flock_type", "glusterfs.flock.type", FT_UINT32, BASE_DEC,
				VALS(glusterfs_lk_type_names), 0, NULL, HFILL }
		},
		{ &hf_glusterfs_flock_whence,
			{ "ia_flock_whence", "glusterfs.flock.whence", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_flock_start,
			{ "ia_flock_start", "glusterfs.flock.start", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_flock_len,
			{ "ia_flock_len", "glusterfs.flock.len", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_flock_pid,
			{ "ia_flock_pid", "glusterfs.flock.pid", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_flock_owner,
			{ "ia_flock_owner", "glusterfs.flock.owner", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},

		/* FIXME: these statfs fields need a better name*/
		{ &hf_glusterfs_bsize,
			{ "bsize", "glusterfs.statfs.bsize", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_frsize,
			{ "frsize", "glusterfs.statfs.frsize", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_blocks,
			{ "blocks", "glusterfs.statfs.blocks", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_bfree,
			{ "bfree", "glusterfs.statfs.bfree", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_bavail,
			{ "bavail", "glusterfs.statfs.bavail", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_files,
			{ "files", "glusterfs.statfs.files", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_ffree,
			{ "ffree", "glusterfs.statfs.ffree", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_favail,
			{ "favail", "glusterfs.statfs.favail", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_id,
			{ "fsid", "glusterfs.statfs.fsid", FT_UINT64, BASE_HEX,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_flag,
			{ "flag", "glusterfs.statfs.flag", FT_UINT64, BASE_HEX,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_namemax,
			{ "namemax", "glusterfs.statfs.namemax", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_setattr_valid,
			{ "valid", "glusterfs.setattr.valid", FT_UINT32, BASE_HEX,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_xflags,
			{ "XFlags", "glusterfs.xflags", FT_UINT32, BASE_OCT,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_oldbname,
			{ "OldBasename", "glusterfs.oldbname", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_newbname,
			{ "NewBasename", "glusterfs.newbname", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_name,
			{ "Name", "glusterfs.name", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_glusterfs_yncdir_data,
			{ "Data", "glusterfs.fsyncdir.data", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		/* For entry an fentry lk */
		{ &hf_glusterfs_entrylk_namelen,
			{ "File Descriptor", "glusterfs.entrylk.namelen", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
	};

	/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_glusterfs,
		&ett_glusterfs_flags,
		&ett_glusterfs_mode,
		&ett_glusterfs_entry,
		&ett_glusterfs_iatt,
		&ett_glusterfs_flock,
		&ett_gluster_dict,
		&ett_gluster_dict_items
	};

	/* Register the protocol name and description */
	proto_glusterfs = proto_register_protocol("GlusterFS", "GlusterFS",
								"glusterfs");
	proto_register_subtree_array(ett, array_length(ett));
	proto_register_field_array(proto_glusterfs, hf, array_length(hf));
}

void
proto_reg_handoff_glusterfs(void)
{
	rpc_init_prog(proto_glusterfs, GLUSTER3_1_FOP_PROGRAM, ett_glusterfs);
	rpc_init_proc_table(GLUSTER3_1_FOP_PROGRAM, 310, glusterfs3_1_fop_proc,
							hf_glusterfs_proc);
	rpc_init_proc_table(GLUSTER3_1_FOP_PROGRAM, 330, glusterfs3_3_fop_proc,
							hf_glusterfs_proc);

}

