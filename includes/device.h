#ifndef __jyos_device_h_
#define __jyos_device_h_

#include <datastructs/hstr.h>
#include <datastructs/jlist.h>
#include <stdarg.h>

#define DEVICE_NAME_MAXLEN    32


struct device{
    
  struct list_header  dev_list;
  struct device       *parent;
  struct hash_str     name;

  char                dev_name[DEVICE_NAME_MAXLEN];
  void                *underlay;
  void                *fs_node;

  int (*read)(
      struct device *this,
      void   *buffer,
      unsigned int offset,
      unsigned int len);

  int (*write)(
      struct device *this,
      void   *buffer,
      unsigned int offset,
      unsigned int len);

};

void device_init();

struct device *device_add
  (struct device* parent, void* underlay, char* name_fmt, ...);

void device_remove(struct device *dev);

#endif
