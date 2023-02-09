#ifndef __jyos_diretn_h_
#define __jyos_diretn_h_

struct dirent{

  unsigned int  d_type;
  unsigned int  d_offset;
  unsigned int  d_nlen;
  char          *d_name;
};


#endif
