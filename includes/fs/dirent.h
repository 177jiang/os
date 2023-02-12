#ifndef __jyos_diretn_h_
#define __jyos_diretn_h_

#define VFS_DIRENT_MAXLEN       256

struct dirent{

  unsigned int  d_type;
  unsigned int  d_offset;
  unsigned int  d_nlen;
  char          d_name[VFS_DIRENT_MAXLEN];
};


#endif
