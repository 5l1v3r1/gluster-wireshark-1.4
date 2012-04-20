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
static gint proto_gluster_fs = -1;
static gint proto_gluster3_1_fop = -1;

/* programs and procedures */
static gint hf_gluster_fs_proc = -1;
static gint hf_gluster3_1_fop_proc = -1;

/* fields used by multiple programs/procedures */
static gint hf_gluster_pargfid = -1;
static gint hf_gluster_path = -1;
static gint hf_gluster_bname = -1;
static gint hf_gluster_fd = -1;
static gint hf_gluster_offset = -1;
static gint hf_gluster_size = -1;
static gint hf_gluster_flags = -1;
static gint hf_gluster_volume = -1;
static gint hf_gluster_cmd = -1;
static gint hf_gluster_type = -1;

/* gf_iatt */
static gint hf_gluster_ia_ino = -1;
static gint hf_gluster_ia_dev = -1;
static gint hf_gluster_mode = -1;
static gint hf_gluster_ia_nlink = -1;
static gint hf_gluster_ia_uid = -1;
static gint hf_gluster_ia_gid = -1;
static gint hf_gluster_ia_rdev = -1;
static gint hf_gluster_ia_size = -1;
static gint hf_gluster_ia_blksize = -1;
static gint hf_gluster_ia_blocks = -1;
static gint hf_gluster_ia_atime = -1;
static gint hf_gluster_ia_atime_nsec = -1;
static gint hf_gluster_ia_mtime = -1;
static gint hf_gluster_ia_mtime_nsec = -1;
static gint hf_gluster_ia_ctime = -1;
static gint hf_gluster_ia_ctime_nsec = -1;

/* gf_flock */
static gint hf_gluster_flock_type = -1;
static gint hf_gluster_flock_whence = -1;
static gint hf_gluster_flock_start = -1;
static gint hf_gluster_flock_len = -1;
static gint hf_gluster_flock_pid = -1;
static gint hf_gluster_flock_owner = -1;

/* statfs */
static gint hf_gluster_bsize = -1;
static gint hf_gluster_frsize = -1;
static gint hf_gluster_blocks = -1;
static gint hf_gluster_bfree = -1;
static gint hf_gluster_bavail = -1;
static gint hf_gluster_files = -1;
static gint hf_gluster_ffree = -1;
static gint hf_gluster_favail = -1;
static gint hf_gluster_fsid = -1;
static gint hf_gluster_flag = -1;
static gint hf_gluster_namemax = -1;

static gint hf_gluster_setattr_valid = -1;

/* Initialize the subtree pointers */
static gint ett_gluster_fs = -1;
static gint ett_gluster3_1_fop = -1;
static gint ett_gluster_iatt = -1;
static gint ett_gluster_flock = -1;

/*
 * from rpc/xdr/src/glusterfs3-xdr.c:xdr_gf_iatt()
 * FIXME: this may be inclomplete, different code-paths may require different
 * encoding/decoding.
 */
static int
gluster_rpc_dissect_gf_iatt(proto_tree *tree, tvbuff_t *tvb, int offset)
{
	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_gfid, offset, 16,
								FALSE, NULL);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_ia_ino, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_ia_dev, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_mode, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_ia_nlink, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_ia_uid, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_ia_gid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_ia_rdev, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_ia_size, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_ia_blksize, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_ia_blocks, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_ia_atime, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_ia_atime_nsec, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_ia_mtime, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_ia_mtime_nsec, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_ia_ctime, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_ia_ctime_nsec, offset);

	return offset;
}

static int
gluster_rpc_dissect_gf_flock(proto_tree *tree, tvbuff_t *tvb, int offset)
{
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_flock_type, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_flock_whence, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_flock_start, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_flock_len, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_flock_pid, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_flock_owner, offset);

	return offset;
}

static int
gluster_rpc_dissect_statfs(proto_tree *tree, tvbuff_t *tvb, int offset)
{
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_bsize, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_frsize, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_blocks, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_bfree, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_bavail, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_files, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_ffree, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_favail, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_fsid, offset);
	/* FIXME: hf_gluster_flag are flags, see 'man 2 statvfs' */
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_flag, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_namemax, offset);

	return offset;
}

/*
 *  372          if (!xdr_int (xdrs, &objp->op_ret))
 *   373                  return FALSE;
 *    374          if (!xdr_int (xdrs, &objp->op_errno))
 *     375                  return FALSE;
 *      376          if (!xdr_gf_iatt (xdrs, &objp->preparent))
 *       377                  return FALSE;
 *        378          if (!xdr_gf_iatt (xdrs, &objp->postparent))
 *
 */
static int
gluster_gfs3_op_unlink_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->preparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_gluster_iatt);
	offset = gluster_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_gluster_iatt);
	offset = gluster_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	return offset;
}

/*
 *  359          if (!xdr_opaque (xdrs, objp->pargfid, 16))
 *   360                  return FALSE;
 *    361          if (!xdr_string (xdrs, &objp->path, ~0))
 *     362                  return FALSE;
 *      363          if (!xdr_string (xdrs, &objp->bname, ~0))
 *       364                  return FALSE;
 *
 */
static int
gluster_gfs3_op_unlink_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar* path = NULL;
	gchar* bname = NULL;
	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_pargfid, offset, 16,
								FALSE, NULL);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_path, offset, &path);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_bname, offset, &bname);
	return offset;
}

static int
gluster_gfs3_op_statfs_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);
	offset = gluster_rpc_dissect_statfs(tree, tvb, offset);
	return offset;
}

static int
gluster_gfs3_op_statfs_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *path = NULL;

	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_gfid, offset, 16,
								FALSE, NULL);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_path, offset, &path);

	return offset;
}

static int
gluster_gfs3_op_flush_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);
	return offset;
}

static int
gluster_gfs3_op_flush_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_gfid, offset, 16,
								FALSE, NULL);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_fd, offset);
	return offset;
}

static int
gluster_gfs3_op_setxattr_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);
	return offset;
}

static int
gluster_gfs3_op_setxattr_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *path = NULL;
	guint flags;

	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_gfid, offset, 16,
								FALSE, NULL);

	/* FIXME: these flags need to be displayed in a sane way */
	flags = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint_format(tree, hf_gluster_flags, tvb, offset, 4, flags, "Flags: 0x%02x", flags);
	offset += 4;

	offset = gluster_rpc_dissect_dict(tree, tvb, hf_gluster_dict, offset);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_path, offset, &path);

	return offset;
}

static int
gluster_gfs3_op_opendir_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_fd, offset);
	return offset;
}

static int
gluster_gfs3_op_opendir_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *path = NULL;

	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_gfid, offset, 16,
								FALSE, NULL);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_path, offset, &path);

	return offset;
}

/* rpc/xdr/src/glusterfs3-xdr.c:xdr_gfs3_create_rsp */
static int
gluster_gfs3_op_create_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->stat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "Stat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_gluster_iatt);
	offset = gluster_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_fd, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->preparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PreParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_gluster_iatt);
	offset = gluster_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_gluster_iatt);
	offset = gluster_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	return offset;
}

/* rpc/xdr/src/glusterfs3-xdr.c:xdr_gfs3_create_req */
static int
gluster_gfs3_op_create_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	guint flags;
	gchar *path = NULL;
	gchar *bname = NULL;

	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_pargfid, offset, 16,
								FALSE, NULL);
	/* FIXME: display as a list of fields */
	flags = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint_format(tree, hf_gluster_flags, tvb, offset, 4, flags, "Flags: 0x%02x", flags);
	offset += 4;

	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_mode, offset);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_path, offset, &path);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_bname, offset, &bname);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_gluster_dict, offset);

	return offset;
}

static int
gluster_gfs3_op_lookup_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);
	/* FIXME: describe this better - gf_iatt (xdrs, &objp->stat */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "Stat IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_gluster_iatt);
	offset = gluster_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	/* FIXME: describe this better - gf_iatt (xdrs, &objp->postparent */
	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "PostParent IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_gluster_iatt);
	offset = gluster_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_gluster_dict, offset);

	return offset;
}

static int
gluster_gfs3_op_lookup_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	gchar *path = NULL;
	gchar *bname = NULL;
	guint flags;

	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_gfid, offset, 16,
								FALSE, NULL);
	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_pargfid, offset, 16,
								FALSE, NULL);
#if 0 /* example from epan/dissectors/packet-tcp.c */
    tf = proto_tree_add_uint_format(tcp_tree, hf_tcp_flags, tvb, offset + 13, 1,
        tcph->th_flags, "Flags: 0x%02x (%s)", tcph->th_flags, flags_strbuf->str);
    field_tree = proto_item_add_subtree(tf, ett_tcp_flags);
    proto_tree_add_boolean(field_tree, hf_tcp_flags_cwr, tvb, offset + 13, 1, tcph->th_flags);
    proto_tree_add_boolean(field_tree, hf_tcp_flags_ecn, tvb, offset + 13, 1, tcph->th_flags);
    proto_tree_add_boolean(field_tree, hf_tcp_flags_urg, tvb, offset + 13, 1, tcph->th_flags);
    proto_tree_add_boolean(field_tree, hf_tcp_flags_ack, tvb, offset + 13, 1, tcph->th_flags);
    proto_tree_add_boolean(field_tree, hf_tcp_flags_push, tvb, offset + 13, 1, tcph->th_flags);
#endif
	/* FIXME: display as a list of fields */
	flags = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint_format(tree, hf_gluster_flags, tvb, offset, 4, flags, "Flags: 0x%02x", flags);
	offset += 4;

	offset = dissect_rpc_string(tvb, tree, hf_gluster_path, offset, &path);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_bname, offset, &bname);
	offset = gluster_rpc_dissect_dict(tree, tvb, hf_gluster_dict, offset);
	
	return offset;
}

static int
gluster_gfs3_op_inodelk_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);

	return offset;
}

static int
gluster_gfs3_op_inodelk_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *flock_item;
	proto_tree *flock_tree;
	gchar* path = NULL;
	gchar* volume = NULL;

	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_gfid, offset, 16,
								FALSE, NULL);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_cmd, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_type, offset);

	flock_item = proto_tree_add_text(tree, tvb, offset, -1, "flock");
	flock_tree = proto_item_add_subtree(flock_item, ett_gluster_flock);
	offset = gluster_rpc_dissect_gf_flock(flock_tree, tvb, offset);

	offset = dissect_rpc_string(tvb, tree, hf_gluster_path, offset, &path);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_volume, offset, &volume);
	return offset;
}

static int
gluster_gfs3_op_readdirp_reply(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);

	if (tree)
		proto_tree_add_text(tree, tvb, offset, -1, "FIXME: The data that follows is a xdr_pointer from xdr_gfs3_dirplist");

	return offset;
}

static int
gluster_gfs3_op_readdirp_call(tvbuff_t *tvb, int offset,
				packet_info *pinfo _U_, proto_tree *tree)
{
	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_gfid, offset, 16,
								FALSE, NULL);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_fd, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_gluster_offset, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_size, offset);

	return offset;
}

static int
gluster_gfs3_op_setattr_reply(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;

	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_ret, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_op_errno, offset);

	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "StatPre IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_gluster_iatt);
	offset = gluster_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "StatPost IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_gluster_iatt);
	offset = gluster_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	return offset;
}

static int
gluster_gfs3_op_setattr_call(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
	proto_item *iatt_item;
	proto_tree *iatt_tree;
	gchar *path = NULL;

	offset = dissect_rpc_bytes(tvb, tree, hf_gluster_gfid, offset, 16,
								FALSE, NULL);

	iatt_item = proto_tree_add_text(tree, tvb, offset, -1, "stbuff IATT");
	iatt_tree = proto_item_add_subtree(iatt_item, ett_gluster_iatt);
	offset = gluster_rpc_dissect_gf_iatt(iatt_tree, tvb, offset);

	/* FIXME: hf_gluster_setattr_valid is a flag
         * see libglusterfs/src/xlator.h, #defines for GF_SET_ATTR_*
         */
	offset = dissect_rpc_uint32(tvb, tree, hf_gluster_setattr_valid, offset);
	offset = dissect_rpc_string(tvb, tree, hf_gluster_path, offset, &path);

	return offset;
}

/*
 * procedures for GLUSTERFS_PROGRAM "GlusterFS Mops"
 *
 * This seems to be spread over multiple files (are Call/Reply seperated?)
 * - xlators/mgmt/glusterd/src/glusterd-rpc-ops.c
 * - glusterfsd/src/glusterfsd-mgmt.c
 */
static const vsff gluster_fs_proc[] = {
	{ GD_MGMT_NULL, "NULL", NULL, NULL },
	{ GD_MGMT_BRICK_OP, "BRICK_OP", NULL, NULL },
	{ GF_BRICK_NULL, "NULL", NULL, NULL },
	{ GF_BRICK_TERMINATE, "TERMINATE", NULL, NULL },
	{ GF_BRICK_XLATOR_INFO, "TRANSLATOR INFO", NULL, NULL },
	{ 0, NULL, NULL, NULL }
};
static const value_string gluster_fs_proc_vals[] = {
	{ GD_MGMT_NULL, "NULL" },
	{ GD_MGMT_BRICK_OP, "BRICK_OP" },
	{ GF_BRICK_NULL, "NULL" },
	{ GF_BRICK_TERMINATE, "TERMINATE" },
	{ GF_BRICK_XLATOR_INFO, "TRANSLATOR INFO" },
	{ 0, NULL }
};

/*
 * GLUSTER3_1_FOP_PROGRAM
 * - xlators/protocol/client/src/client3_1-fops.c
 * - xlators/protocol/server/src/server3_1-fops.c
 */
static const vsff gluster3_1_fop_proc[] = {
	{ GFS3_OP_NULL, "NULL", NULL, NULL },
	{ GFS3_OP_STAT, "STAT", NULL, NULL },
	{ GFS3_OP_READLINK, "READLINK", NULL, NULL },
	{ GFS3_OP_MKNOD, "MKNOD", NULL, NULL },
	{ GFS3_OP_MKDIR, "MKDIR", NULL, NULL },
	{
		GFS3_OP_UNLINK, "UNLINK",
		gluster_gfs3_op_unlink_call, gluster_gfs3_op_unlink_reply
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
		gluster_gfs3_op_statfs_call, gluster_gfs3_op_statfs_reply
	},
	{
		GFS3_OP_FLUSH, "FLUSH",
		gluster_gfs3_op_flush_call, gluster_gfs3_op_flush_reply
	},
	{ GFS3_OP_FSYNC, "FSYNC", NULL, NULL },
	{
		GFS3_OP_SETXATTR, "SETXATTR",
		gluster_gfs3_op_setxattr_call, gluster_gfs3_op_setxattr_reply
	},
	{ GFS3_OP_GETXATTR, "GETXATTR", NULL, NULL },
	{ GFS3_OP_REMOVEXATTR, "REMOVEXATTR", NULL, NULL },
	{
		GFS3_OP_OPENDIR, "OPENDIR",
		gluster_gfs3_op_opendir_call, gluster_gfs3_op_opendir_reply
	},
	{ GFS3_OP_FSYNCDIR, "FSYNCDIR", NULL, NULL },
	{ GFS3_OP_ACCESS, "ACCESS", NULL, NULL },
	{
		GFS3_OP_CREATE, "CREATE",
		gluster_gfs3_op_create_call, gluster_gfs3_op_create_reply
	},
	{ GFS3_OP_FTRUNCATE, "FTRUNCATE", NULL, NULL },
	{ GFS3_OP_FSTAT, "FSTAT", NULL, NULL },
	{ GFS3_OP_LK, "LK", NULL, NULL },
	{
		GFS3_OP_LOOKUP, "LOOKUP",
		gluster_gfs3_op_lookup_call, gluster_gfs3_op_lookup_reply
	},
	{ GFS3_OP_READDIR, "READDIR", NULL, NULL },
	{
		GFS3_OP_INODELK, "INODELK",
		gluster_gfs3_op_inodelk_call, gluster_gfs3_op_inodelk_reply
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
		gluster_gfs3_op_setattr_call, gluster_gfs3_op_setattr_reply
	},
	{
		GFS3_OP_FSETATTR, "FSETATTR",
		/* SETATTR and SETFATTS calls and reply are encoded the same */
		gluster_gfs3_op_setattr_call, gluster_gfs3_op_setattr_reply
	},
	{
		GFS3_OP_READDIRP, "READDIRP",
		gluster_gfs3_op_readdirp_call, gluster_gfs3_op_readdirp_reply
	},
	{ GFS3_OP_RELEASE, "RELEASE", NULL, NULL },
	{ GFS3_OP_RELEASEDIR, "RELEASEDIR", NULL, NULL },
	{ 0, NULL, NULL, NULL }
};
static const value_string gluster3_1_fop_proc_vals[] = {
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


void
proto_register_glusterfs(void)
{
	/* Setup list of header fields  See Section 1.6.1 for details */
	static hf_register_info hf[] = {
		/* programs */
		{ &hf_gluster_fs_proc,
			{ "GlusterFS Mops", "gluster.mops", FT_UINT32,
				BASE_DEC, VALS(gluster_fs_proc_vals), 0, NULL,
				HFILL }
		},
		{ &hf_gluster3_1_fop_proc,
			{ "GlusterFS", "glusterfs", FT_UINT32, 	BASE_DEC,
				VALS(gluster3_1_fop_proc_vals), 0, NULL, HFILL }
		},
		/* fields used by multiple programs/procedures */
		{ &hf_gluster_pargfid,
			{ "PARGFID (FIXME?)", "gluster.pargfid", FT_BYTES,
				BASE_NONE, NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_path,
			{ "Path", "gluster.path", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_bname,
			{ "Basename", "gluster.bname", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_fd,
			{ "File Descriptor", "gluster.fd", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_offset,
			{ "Offset", "gluster.offset", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_size,
			{ "Size", "gluster.size", FT_UINT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_flags,
			{ "Flags", "gluster.flags", FT_UINT32, BASE_OCT,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_type,
			{ "Type", "gluster.type", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_cmd,
			{ "Command", "gluster.cmd", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_volume,
			{ "Volume", "gluster.volume", FT_STRING, BASE_NONE,
				NULL, 0, NULL, HFILL }
		},
		/* the IATT structure */
		{ &hf_gluster_ia_ino,
			{ "ia_ino", "gluster.ia_ino", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_ia_dev,
			{ "ia_dev", "gluster.ia_dev", FT_UINT64, BASE_HEX,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_mode,
			{ "mode", "gluster.mode", FT_UINT32, BASE_OCT,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_ia_nlink,
			{ "ia_nlink", "gluster.ia_nlink", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_ia_uid,
			{ "ia_uid", "gluster.ia_uid", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_ia_gid,
			{ "ia_gid", "gluster.ia_gid", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_ia_rdev,
			{ "ia_rdev", "gluster.ia_rdev", FT_UINT64, BASE_HEX,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_ia_size,
			{ "ia_size", "gluster.ia_size", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_ia_blksize,
			{ "ia_blksize", "gluster.ia_blksize", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_ia_blocks,
			{ "ia_blocks", "gluster.ia_blocks", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_ia_atime,
			{ "ia_time", "gluster.is_time", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_ia_atime_nsec,
			{ "ia_atime_nsec", "gluster.ia_atime_nsec", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_ia_mtime,
			{ "ia_mtime", "gluster.is_mtime", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_ia_mtime_nsec,
			{ "ia_mtime_msec", "gluster.is_mtime_nsec", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_ia_ctime,
			{ "ia_ctime", "gluster.ia_ctime", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_ia_ctime_nsec,
			{ "ia_ctime_nsec", "gluster.ia_ctime_nsec", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},

		/* gf_flock */
		{ &hf_gluster_flock_type,
			{ "ia_flock_type", "gluster.flock.type", FT_UINT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_flock_whence,
			{ "ia_flock_whence", "gluster.flock.whence", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_flock_start,
			{ "ia_flock_start", "gluster.flock.start", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_flock_len,
			{ "ia_flock_len", "gluster.flock.len", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_flock_pid,
			{ "ia_flock_pid", "gluster.flock.pid", FT_INT32, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_flock_owner,
			{ "ia_flock_owner", "gluster.flock.owner", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},

		/* FIXME: these statfs fields need a better name*/
		{ &hf_gluster_bsize,
			{ "bsize", "gluster.statfs.bsize", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_frsize,
			{ "frsize", "gluster.statfs.frsize", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_blocks,
			{ "blocks", "gluster.statfs.blocks", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_bfree,
			{ "bfree", "gluster.statfs.bfree", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_bavail,
			{ "bavail", "gluster.statfs.bavail", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_files,
			{ "files", "gluster.statfs.files", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_ffree,
			{ "ffree", "gluster.statfs.ffree", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_favail,
			{ "favail", "gluster.statfs.favail", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_fsid,
			{ "fsid", "gluster.statfs.fsid", FT_UINT64, BASE_HEX,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_flag,
			{ "flag", "gluster.statfs.flag", FT_UINT64, BASE_HEX,
				NULL, 0, NULL, HFILL }
		},
		{ &hf_gluster_namemax,
			{ "namemax", "gluster.statfs.namemax", FT_UINT64, BASE_DEC,
				NULL, 0, NULL, HFILL }
		},

		{ &hf_gluster_setattr_valid,
			{ "valid", "gluster.setattr.valid", FT_UINT32, BASE_HEX,
				NULL, 0, NULL, HFILL }
		}
	};

	/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_gluster_fs,
		&ett_gluster3_1_fop,
		&ett_gluster_iatt,
		&ett_gluster_flock
	};

	/* Register the protocol name and description */
	proto_register_subtree_array(ett, array_length(ett));
	proto_register_field_array(proto_gluster, hf, array_length(hf));

	proto_gluster_fs = proto_register_protocol("GlusterFS Mops",
					"GlusterFS Mops", "gluster-mops");

	proto_gluster3_1_fop = proto_register_protocol("GlusterFS",
				"GlusterFS", "glusterfs");
}

void
proto_reg_handoff_glusterfs(void)
{
	rpc_init_prog(proto_gluster_fs, GLUSTERFS_PROGRAM, ett_gluster_fs);
	rpc_init_proc_table(GLUSTERFS_PROGRAM, 1, gluster_fs_proc,
							hf_gluster_fs_proc);

	rpc_init_prog(proto_gluster3_1_fop, GLUSTER3_1_FOP_PROGRAM, ett_gluster3_1_fop);
	rpc_init_proc_table(GLUSTER3_1_FOP_PROGRAM, 310, gluster3_1_fop_proc,
							hf_gluster3_1_fop_proc);
}
