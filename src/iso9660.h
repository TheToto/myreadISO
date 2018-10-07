/*
 **    This file is part of my_read_iso.
 **
 **    my_read_iso is free software; you can redistribute it and/or modify
 **    it under the terms of the GNU General Public License as published by
 **    the Free Software Foundation; either version 2 of the License, or
 **    (at your option) any later version.
 **
 **    my_read_iso is distributed in the hope that it will be useful,
 **    but WITHOUT ANY WARRANTY; without even the implied warranty of
 **    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **    GNU General Public License for more details.
 **
 **    You should have received a copy of the GNU General Public License
 **    along with my_read_iso; if not, write to the Free Software
 **    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 **
 **    Copyright Alexandre Becoulet, 2003
 **    contact : alexandre.becoulet@epita.fr
 */

#ifndef ISO9660_H
# define ISO9660_H

# include <stdint.h>

/* Structures used for twin values */

struct endian32
{
  uint32_t le;  /* little endian value */
  uint32_t be;  /* big endian value */
} __attribute__ ((packed));

struct endian16
{
  uint16_t le;  /* little endian value */
  uint16_t be;  /* big endian value */
} __attribute__ ((packed));

# define ISO_BLOCK_SIZE 2048
# define ISO_MAX_DIR_DEPTH 8

/* ISO9660 Path table structure */

struct iso_path_table_le
{
  uint8_t    idf_len; /* Identifier name length */
  uint8_t    ext_size;  /* Extended attribute record length */
  uint32_t   data_blk;  /* File data block index */
  uint16_t   parent_dir;  /* Number of the parent dir */
  /*char     idf[0];  directory name */
} __attribute__ ((packed));

/* ISO9660 directory structure */

# define ISO_DATE_LEN  7

enum iso_file_type
{
  ISO_FILE_HIDDEN   = 1,  /* File is Hidden */
  ISO_FILE_ISDIR    = 2,  /* Entry is a Directory */
  ISO_FILE_ASSOCIAT = 4,  /* Entry is an Associated */
  ISO_FILE_USEEXT   = 8,
    /* Information is structured according to the extended attribute record */
  ISO_FILE_USEPERM  = 16,
    /* Permissions are specified in the extended attribute record */
  ISO_FILE_MULTIDIR = 128 /* File has more than one directory record */
};

struct iso_dir
{
  uint8_t   dir_size;  /* Length of directory record */
  uint8_t   ext_size;  /* Extended attribute record length */
  struct endian32  data_blk;  /* File data block index */
  struct endian32  file_size; /* File size */
  char      date[ISO_DATE_LEN];
  uint8_t   type;    /* File type (enum iso_file_type) */

  /* only valid if the file is recorded in interleave mode */
  uint8_t   unit_size; /* File Unit Size */
  uint8_t   gap_size;  /* Interleave Gap Size */

  struct endian16  vol_seq;
  uint8_t   idf_len;
  /*char    idf[0];     file name */
} __attribute__ ((packed));

/* ISO9660 Primary volume descriptor structure */

# define ISO_PRIM_VOLDESC_BLOCK 16

# define ISO_SYSIDF_LEN 32
# define ISO_VOLIDF_LEN 32
# define ISO_VOLSET_LEN 128
# define ISO_PUBIDF_LEN 128
# define ISO_DPREP_LEN  128
# define ISO_APP_LEN  128
# define ISO_CPRFIL_LEN 37
# define ISO_ABSFIL_LEN 37
# define ISO_BIBFIL_LEN 37
# define ISO_LDATE_LEN  17

struct iso_prim_voldesc
{
  uint8_t   vol_desc_type;   /* Volume Descriptor Type (1) */
  char      std_identifier[5]; /* Standard Identifier (CD001) */
  uint8_t   vol_desc_version;  /* Volume Descriptor Version (1) */
  uint8_t   unused1; /* Unused Field */
  char      syidf[ISO_SYSIDF_LEN]; /* System Identifier */
  char      vol_idf[ISO_VOLIDF_LEN]; /* Volume Identifier */
  uint8_t   unused2[8]; /* Unused Field */
  struct endian32  vol_blk_count;
    /* Number of logical blocks in the Volume (LE)*/
  uint8_t   unused4[32]; /* Unused Field */
  struct endian16  vol_set_size; /* The Volume Set size of the Volume */
  struct endian16  vol_seq_num; /* The number of the volume in the Set */
  struct endian16  vol_blk_size; /* The size in bytes of a Logical Block */
  struct endian32  path_table_size; /* Length in bytes of the path table */
  uint32_t  le_path_table_blk; /* LittleEndian path table block index */
  uint32_t  le_opath_table_blk;
    /* LittleEndian optional path table block index */
  uint32_t  be_path_table_blk; /* BigEndian path table block index */
  uint32_t  be_opath_table_blk; /* BigEndian optional path table block index */

  struct iso_dir   root_dir;    /* Root directory entry */
  uint8_t unused5[34 - sizeof (struct iso_dir)];/* padding */

  char    volset_idf[ISO_VOLSET_LEN];  /* Name of the multiple volume set */
  char    pub_idf[ISO_PUBIDF_LEN]; /* Publisher name */
  char    dprep_idf[ISO_DPREP_LEN];  /* Data preparer name */
  char    app_idf[ISO_APP_LEN];  /* Application name */

  char    copyright_file[ISO_CPRFIL_LEN]; /* Copyright file name in root dir */
  char    abstract_file[ISO_ABSFIL_LEN]; /* Abstract file name in root dir */
  char    bibli_file[ISO_BIBFIL_LEN]; /* Bibliographic file name in root dir */
  char    date_creat[ISO_LDATE_LEN]; /* Creation date */
  char    date_modif[ISO_LDATE_LEN]; /* Modification date */
  char    date_expir[ISO_LDATE_LEN]; /* Expiration date */
  char    date_effect[ISO_LDATE_LEN]; /* Effective date */
  uint8_t filestrutc_version;  /* File Structure Version (1) */
} __attribute__ ((packed));

#endif /* !ISO9660_H */
