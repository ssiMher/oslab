#include "klib.h"
#include "fs.h"
#include "disk.h"
#include "proc.h"

#ifdef EASY_FS

#define MAX_FILE  (SECTSIZE / sizeof(dinode_t))
#define MAX_DEV   16
#define MAX_INODE (MAX_FILE + MAX_DEV)

// On disk inode
typedef struct dinode {
  uint32_t start_sect;
  uint32_t length;
  char name[MAX_NAME + 1];
} dinode_t;

// On OS inode, dinode with special info
struct inode {
  int valid;
  int type;
  int dev; // dev_id if type==TYPE_DEV
  dinode_t dinode;
};

static inode_t inodes[MAX_INODE];

void init_fs() {
  dinode_t buf[MAX_FILE];
  read_disk(buf, 256);
  for (int i = 0; i < MAX_FILE; ++i) {
    inodes[i].valid = 1;
    inodes[i].type = TYPE_FILE;
    inodes[i].dinode = buf[i];
  }
}

inode_t *iopen(const char *path, int type) {
  for (int i = 0; i < MAX_INODE; ++i) {
    if (!inodes[i].valid) continue;
    assert(path!=0);
    if (strcmp(path, inodes[i].dinode.name) == 0) {
      return &inodes[i];
    }
  }
  return NULL;
}

int iread(inode_t *inode, uint32_t off, void *buf, uint32_t len) {
  assert(inode);
  char *cbuf = buf;
  char dbuf[SECTSIZE];
  uint32_t curr = -1;
  uint32_t total_len = inode->dinode.length;
  uint32_t st_sect = inode->dinode.start_sect;
  int i;
  for (i = 0; i < len && off < total_len; ++i, ++off) {
    if (curr != off / SECTSIZE) {
      read_disk(dbuf, st_sect + off / SECTSIZE);
      curr = off / SECTSIZE;
    }
    *cbuf++ = dbuf[off % SECTSIZE];
  }
  return i;
}

void iadddev(const char *name, int id) {
  assert(id < MAX_DEV);
  inode_t *inode = &inodes[MAX_FILE + id];
  inode->valid = 1;
  inode->type = TYPE_DEV;
  inode->dev = id;
  strcpy(inode->dinode.name, name);
}

uint32_t isize(inode_t *inode) {
  return inode->dinode.length;
}

int itype(inode_t *inode) {
  return inode->type;
}

uint32_t ino(inode_t *inode) {
  return inode - inodes;
}

int idevid(inode_t *inode) {
  return inode->type == TYPE_DEV ? inode->dev : -1;
}

int iwrite(inode_t *inode, uint32_t off, const void *buf, uint32_t len) {
  panic("write doesn't support");
}

void itrunc(inode_t *inode) {
  panic("trunc doesn't support");
}

inode_t *idup(inode_t *inode) {
  return inode;
}

void iclose(inode_t *inode) { /* do nothing */ }

int iremove(const char *path) {
  panic("remove doesn't support");
}

#else

#define DISK_SIZE (128 * 1024 * 1024)
#define BLK_NUM   (DISK_SIZE / BLK_SIZE)

#define NDIRECT   12
#define NINDIRECT (BLK_SIZE / sizeof(uint32_t))

#define IPERBLK   (BLK_SIZE / sizeof(dinode_t)) // inode num per blk

// super block
typedef struct super_block {
  uint32_t bitmap; // block num of bitmap
  uint32_t istart; // start block no of inode blocks
  uint32_t inum;   // total inode num
  uint32_t root;   // inode no of root dir
} sb_t;

// On disk inode
typedef struct dinode {
  uint32_t type;   // file type
  uint32_t device; // if it is a dev, its dev_id
  uint32_t size;   // file size
  uint32_t addrs[NDIRECT + 1]; // data block addresses, 12 direct and 1 indirect
} dinode_t;

struct inode {
  int no;
  int ref;
  int del;
  dinode_t dinode;
};

#define SUPER_BLOCK 32
static sb_t sb;

void init_fs() {
  bread(&sb, sizeof(sb), SUPER_BLOCK, 0);
}

#define I2BLKNO(no)  (sb.istart + no / IPERBLK)
#define I2BLKOFF(no) ((no % IPERBLK) * sizeof(dinode_t))

static void diread(dinode_t *di, uint32_t no) {
  bread(di, sizeof(dinode_t), I2BLKNO(no), I2BLKOFF(no));
}

static void diwrite(const dinode_t *di, uint32_t no) {
  bwrite(di, sizeof(dinode_t), I2BLKNO(no), I2BLKOFF(no));
}

static uint32_t dialloc(int type) {
  // Lab3-2: iterate all dinode, find a empty one (type==TYPE_NONE)
  // set type, clean other infos and return its no (remember to write back)
  // if no empty one, just abort
  // note that first (0th) inode always unused, because dirent's inode 0 mark invalid
  dinode_t dinode;
  for (uint32_t i = 1; i < sb.inum; ++i) {
    diread(&dinode, i);
    if(dinode.type==TYPE_NONE){
      memset(&dinode, 0, sizeof dinode);
      dinode.type = type;
      diwrite(&dinode, i);
      return i;
    }
  }
  assert(0);
}

static void difree(uint32_t no) {
  dinode_t dinode;
  memset(&dinode, 0, sizeof dinode);
  diwrite(&dinode, no);
}

static uint32_t balloc() {
  // Lab3-2: iterate bitmap, find one free block
  // set the bit, clean the blk (can call bzero) and return its no
  // if no free block, just abort
  uint32_t byte;
  for (int i = 0; i < BLK_NUM / 32; ++i) {
    bread(&byte, 4, sb.bitmap, i * 4);
    if (byte != 0xffffffff) {
      //printf("balloc() byte = %x\n",byte);
      int j = 1, k = 0;
      //int j = 0;
      //if(byte != 0){
        //for(;j<= 0xffff;j= j << 1){
        for(;j< 32;j++){
          //printf("balloc() j = %d, byte = %x\n", j, byte);
          k = j;
          j=1<<(j-1);
          j= j & byte;
          
          //printf("j = %d k = %d\n",j, k);
          if(j==0){
          //if(byte>>j==0){
            j = k;
            bzero(32*i+j);
            //printf("balloc() blk: %d\n", 32*i+j);
            // if(j==0)byte=1;
            // else byte = byte<<1;
            j=1<<(j-1);
            byte = byte | j;
            bwrite(&byte, 4, sb.bitmap, i * 4);
            //printf("balloc() j = %d byte = %x\n",j, byte);
            j = k;
            return 32*i+j;
          }
          j = k;
        }
        
      //}
    }
  }
  // uint32_t byte;
  // for (int i = 0; i < BLK_NUM / 32; ++i) {
  //   bread(&byte, 4, sb.bitmap, i * 4);
  //   if (byte != 0xffffffff) {
  //     //printf("balloc() byte = %x\n",byte);
  //     int j = 0;
  //     //if(byte != 0){
  //       for(;j<32;j++){
  //         if(byte>>j==0){
  //           bzero(32*i+j);
  //           printf("balloc() blk: %d\n", 32*i+j);
  //           if(j==0)byte=1;
  //           else byte = byte<<1;
  //           bwrite(&byte, 4, sb.bitmap, i * 4);
  //           //printf("balloc() j = %d\n",j);
  //           return 32*i+j;
  //         }
  //       }
        
  //     //}
  //   }
  // }
  assert(0);
}

static void bfree(uint32_t blkno) {
  // Lab3-2: clean the bit of blkno in bitmap
  assert(blkno >= 64); // cannot free first 64 block
  int i = blkno/32;
  int j = blkno%32;
  
  uint32_t byte; 
  bread(&byte, 4, sb.bitmap, i * 4);
  //printf("bfree() blk: %d j = %d byte = %x\n",32*i+j, j, byte);
  //printf("bfree() i = %d j = %d byte = %x\n", i, j, byte);
  int b = 0x1;
  if(j==0)byte&=0xfffffffe;
  else{
    b = b << (j-1);
    b=~b;
    //printf("bfree() b = %x\n", b);
    byte&=b;
  }
  //printf("bfree2() blkno = %d j = %d byte = %x\n", blkno, j, byte);
  bwrite(&byte, 4, sb.bitmap, i * 4);
}

#define INODE_NUM 128
static inode_t inodes[INODE_NUM];

static inode_t *iget(uint32_t no) {
  // Lab3-2
  // if there exist one inode whose no is just no, inc its ref and return it
  // otherwise, find a empty inode slot, init it and return it
  // if no empty inode slot, just abort
  for(int i = 0; i < INODE_NUM; i++){
    if(inodes[i].no == no){
      inodes[i].ref++;
      return &inodes[i];
    }
  }
  for(int i = 0; i < INODE_NUM; i++){
    if(inodes[i].ref == 0){
      inodes[i].no = no;
      inodes[i].ref = 1;
      inodes[i].del = 0;
      diread(&inodes[i].dinode, no);
      return &inodes[i];
    }
  }
  assert(0);
}

static void iupdate(inode_t *inode) {
  // Lab3-2: sync the inode->dinode to disk
  // call me EVERYTIME after you edit inode->dinode
  diwrite(&inode->dinode, inode->no);
}


// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return NULL.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = NULL
//
static const char* skipelem(const char *path, char *name) {
  //printf("skipelem()1 path = %s, name = %s\n",path, name);
  const char *s;
  int len;
  while (*path == '/') path++;
  if (*path == 0) return 0;
  s = path;
  while(*path != '/' && *path != 0) path++;
  len = path - s;
  if (len >= MAX_NAME) {
    memcpy(name, s, MAX_NAME);
    name[MAX_NAME] = 0;
  } else {
    memcpy(name, s, len);
    name[len] = 0;
  }
  while (*path == '/') path++;
  //printf("skipelem()2 path = %s, name = %s\n",path, name);
  return path;
}

static void idirinit(inode_t *inode, inode_t *parent) {
  // Lab3-2: init the dir inode, i.e. create . and .. dirent
  assert(inode->dinode.type == TYPE_DIR);
  assert(parent->dinode.type == TYPE_DIR); // both should be dir
  assert(inode->dinode.size == 0); // inode shoule be empty
  //printf("idirinit()\n");
  dirent_t dirent;
  // set .
  dirent.inode = inode->no;
  strcpy(dirent.name, ".");
  iwrite(inode, 0, &dirent, sizeof dirent);
  // set ..
  dirent.inode = parent->no;
  strcpy(dirent.name, "..");
  iwrite(inode, sizeof dirent, &dirent, sizeof dirent);
}

static inode_t *ilookup(inode_t *parent, const char *name, uint32_t *off, int type) {
  // Lab3-2: iterate the parent dir, find a file whose name is name
  // if off is not NULL, store the offset of the dirent_t to it
  // if no such file and type == TYPE_NONE, return NULL
  // if no such file and type != TYPE_NONE, create the file with the type
  //printf("ilookup() type = %d\n",type);
  assert(parent->dinode.type == TYPE_DIR); // parent must be a dir
  dirent_t dirent;
  uint32_t size = parent->dinode.size, empty = size;
  //printf("ilookup() name = %s, type = %d, size = %d\n",name, type, size);
  for (uint32_t i = 0; i < size; i += sizeof dirent) {
    // directory is a file containing a sequence of dirent structures
    iread(parent, i, &dirent, sizeof dirent);
    //iread(parent, i, &dirent, sizeof dirent);
    //printf("ilookup() name = %s\n", dirent.name);
    if (dirent.inode == 0) {
      // a invalid dirent, record the offset (used in create file), then skip
      if (empty == size) empty = i;
      continue;
    }
    //printf("ilookup() name = %s\n", dirent.name);
    // a valid dirent, compare the name
    if(strcmp(dirent.name, name)==0){
      //printf("ilookup() name = %s\n", name);
      inode_t* inod = iget(dirent.inode);
      if(off!=NULL)*off = i;
      return inod;
    }
  }
  // not found
  if (type == TYPE_NONE) return NULL;
  // need to create the file, first alloc inode, then init dirent, write it to parent
  // if you create a dir, remember to init it's . and ..
  inode_t* inod = iget(dialloc(type));
  if(type == TYPE_DIR){
    idirinit(inod, parent);
  }
  //dirent_t dirent;
  // set .
  //printf("ilookup()don't match\n");
  dirent.inode = inod->no;
  strcpy(dirent.name, name);
  iwrite(parent, empty, &dirent, sizeof dirent);
  if(off!=NULL)*off = empty;
  return inod;
}

static inode_t *iopen_parent(const char *path, char *name) {
  // Lab3-2: open the parent dir of path, store the basename to name
  // if no such parent, return NULL
  //printf("iopen_parent() path = %s, name = %s\n",path, name);
  inode_t *ip, *next;
  // set search starting inode
  if (path[0] == '/') {
    ip = iget(sb.root);
  } else {
    ip = idup(proc_curr()->cwd);
  }
  assert(ip);
  while ((path = skipelem(path, name))) {
    // curr round: need to search name in ip
    //printf("iopen_parent() path = %s, name = %s\n",path, name);
    if (ip->dinode.type != TYPE_DIR) {
      // not dir, cannot search
      iclose(ip);
      //printf("iopen_parent() NULL1\n");
      return NULL;
    }
    if (*path == 0) {
      // last search, return ip because we just need parent
      return ip;
    }
    // not last search, need to continue to find parent
    next = ilookup(ip, name, NULL, 0);
    if (next == NULL) {
      // name not exist
      iclose(ip);
      //printf("iopen_parent() NULL2\n");
      return NULL;
    }
    iclose(ip);
    ip = next;
  }
  iclose(ip);
  return NULL;
}

inode_t *iopen(const char *path, int type) {
  // Lab3-2: if file exist, open and return it
  // if file not exist and type==TYPE_NONE, return NULL
  // if file not exist and type!=TYPE_NONE, create the file as type
  
  char name[MAX_NAME + 1];
  if (skipelem(path, name) == NULL) {
    // no parent dir for path, path is "" or "/"
    // "" is an invalid path, "/" is root dir
    return path[0] == '/' ? iget(sb.root) : NULL;
  }
  // path do have parent, use iopen_parent and ilookup to open it
  // remember to close the parent inode after you ilookup it
  //char nm[MAX_NAME+1];
  inode_t* parent = iopen_parent(path, name);
  //printf("iopen() path = %s, name = %s, type = %d\n", path, nm, type);
  if(parent==NULL){
    //printf("parent == NULL\n");
    return NULL;
  }
  //printf("iopen() path = %s, type = %d\n", path, type);
  //printf("iopen() type = %d\n",type);
  inode_t* inode = ilookup(parent, name, NULL, type);
  
  iclose(parent);
  //printf("iopen() inode->type = %x\n",inode->dinode.type);
  return inode;
}

static uint32_t iwalk(inode_t *inode, uint32_t no) {
  // return the blkno of the file's data's no th block, if no, alloc it
  if (no < NDIRECT) {
    // direct address
    uint32_t blkno = inode->dinode.addrs[no];
    if(blkno==0){
      inode->dinode.addrs[no] = balloc();
      //int kk = inode->dinode.addrs[no];
      //printf("iwalk1() inode->dinode.addrs[%d] = %d\n", no, kk);
      iupdate(inode);
      blkno = inode->dinode.addrs[no];
    }
    return blkno;
  }
  no -= NDIRECT;
  if (no < NINDIRECT) {
    // indirect address
    uint32_t blkno = inode->dinode.addrs[NDIRECT];
    if(blkno==0){
      inode->dinode.addrs[NDIRECT] = balloc();
      //int kk = inode->dinode.addrs[NDIRECT];
      //printf("iwalk2() inode->dinode.addrs[%d] = %d\n", NDIRECT, kk);
      iupdate(inode);
      blkno = inode->dinode.addrs[NDIRECT];
    }
    uint32_t undno[1];
    bread(undno, 4, blkno, no*4);
    //printf("iwalk3() bread(dst:%d, 4, no:%d, off:%d)\n", *undno, blkno, no);
    if(*undno==0){
      uint32_t ba = balloc();
      //printf("iwalk4() balloc() ba=%d\n", ba);
      *undno = ba;
      bwrite(undno, 4, blkno, no*4);
      //printf("iwalk3() bwrite(dst:%d, 4, no:%d, off:%d)\n", *undno, blkno, no);
      iupdate(inode);
    }
    return *undno;
  }
  assert(0); // file too big, not need to handle this case
}

int iread(inode_t *inode, uint32_t off, void *buf, uint32_t len) {
  // Lab3-2: read the inode's data [off, MIN(off+len, size)) to buf
  // use iwalk to get the blkno and read blk by blk
  //assert(inode->dinode.size > off);
  //printf("iread() off = %d, size = %d\n", off, inode->dinode.size);
  if(inode->dinode.size<off){
    //printf("iread()\n");
    return -1;
  }
  uint32_t size = inode->dinode.size;
  uint32_t end = (uint32_t)MIN(off+len, size);
  //void* cbuf = buf;
  uint32_t no = off/BLK_SIZE;
  uint32_t blkno = iwalk(inode, off/BLK_SIZE);
  uint32_t limit = BLK_SIZE - off % BLK_SIZE;
  uint32_t start = off % BLK_SIZE;
  uint32_t ret = 0;
  while(off<end){
    no = off / BLK_SIZE;
    blkno = iwalk(inode, no);
    limit = BLK_SIZE - off % BLK_SIZE;
    if(limit > end - off) limit = end - off;
    bread(buf, limit, blkno, start);
    buf+=limit;
    start = 0;
    off+=limit;
    ret += limit;
  }
  return ret;
}

int iwrite(inode_t *inode, uint32_t off, const void *buf, uint32_t len) {
  // Lab3-2: write buf to the inode's data [off, off+len)
  // if off>size, return -1 (can not cross size before write)
  // if off+len>size, update it as new size (but can cross size after write)
  // use iwalk to get the blkno and read blk by blk
  //uint32_t size = inode->dinode.size;
  uint32_t end = off+len;
  //void* cbuf = buf;
  uint32_t no = off/BLK_SIZE;
  uint32_t blkno = iwalk(inode, off/BLK_SIZE);
  uint32_t limit = BLK_SIZE - off % BLK_SIZE;
  uint32_t start = off % BLK_SIZE;
  uint32_t ret = 0;
  while(off<end){
    no = off / BLK_SIZE;
    blkno = iwalk(inode, no);
    limit = BLK_SIZE - off % BLK_SIZE;
    if(limit > end - off) limit = end - off;
    bwrite(buf, limit, blkno, start);
    buf+=limit;
    start = 0;
    off+=limit;
    ret += limit;
  }
  if(end > inode->dinode.size){
    inode->dinode.size = end;
    iupdate(inode);
  }
  return ret;
}

void itrunc(inode_t *inode) {
  // Lab3-2: free all data block used by inode (direct and indirect)
  // mark all address of inode 0 and mark its size 0
  uint32_t no = 0;
  //printf("itrunc() inode %d\n", inode->no);
  while(no < 1036){
  if (no < NDIRECT) {
    // direct address
    //printf("itrunc() inode->addrs[%d]=%d\n",no,inode->dinode.addrs[no]);
    if(inode->dinode.addrs[no]!=0){
      bzero(inode->dinode.addrs[no]);
      bfree(inode->dinode.addrs[no]);
    }
    no++;
    continue;
  }
  no -= NDIRECT;
  if (no < NINDIRECT) {
    // indirect address
    uint32_t blkno = inode->dinode.addrs[NDIRECT];
    if(blkno==0){
      break;
    }
    uint32_t undno[1];
    bread(undno, 4, blkno, no);
    if(*undno!=0){
      //printf("itrunc() inode->addrs[%d]=%d\n",no,*undno);
      bzero(*undno);
      bfree(*undno);
    }
  }
  no +=NDIRECT;
  no++;
  }
  uint32_t blkno = inode->dinode.addrs[NDIRECT];
    if(blkno!=0){
      bzero(blkno);
      bfree(blkno);
    }
  inode->dinode.size = 0;
  iupdate(inode);
}

inode_t *idup(inode_t *inode) {
  assert(inode);
  inode->ref += 1;
  return inode;
}

void iclose(inode_t *inode) {
  assert(inode);
  if (inode->ref == 1 && inode->del) {
    //printf("iclose() remove file\n");
    itrunc(inode);
    difree(inode->no);
  }
  inode->ref -= 1;
}

uint32_t isize(inode_t *inode) {
  return inode->dinode.size;
}

int itype(inode_t *inode) {
  return inode->dinode.type;
}

uint32_t ino(inode_t *inode) {
  return inode->no;
}

int idevid(inode_t *inode) {
  return itype(inode) == TYPE_DEV ? inode->dinode.device : -1;
}

void iadddev(const char *name, int id) {
  inode_t *ip = iopen(name, TYPE_DEV);
  //printf("iadddev() name = %s\n", name);
  //printf("iadddev() ip->type = %d\n", ip->dinode.type);
  assert(ip);
  ip->dinode.device = id;
  iupdate(ip);
  iclose(ip);
}

static int idirempty(inode_t *inode) {
  // Lab3-2: return whether the dir of inode is empty
  // the first two dirent of dir must be . and ..
  // you just need to check whether other dirent are all invalid
  assert(inode->dinode.type == TYPE_DIR);
  dirent_t dirent;
  uint32_t size = inode->dinode.size;
  for (uint32_t i = sizeof dirent * 2; i < size; i += sizeof dirent) {
    // directory is a file containing a sequence of dirent structures
    if(iread(inode, i, &dirent, sizeof dirent)==-1)continue;
    //iread(inode, i, &dirent, sizeof dirent);
    if (dirent.inode != 0) {
      // a invalid dirent, record the offset (used in create file), then skip
      //printf("idirempty() dir not empty\n");
      return -1;
    }
  }
  //printf("idirempty() dir is empty\n");
  return 0;
}

int iremove(const char *path) {
  // Lab3-2: remove the file, return 0 on success, otherwise -1
  // first open its parent, if no parent, return -1
  // then find file in parent, if not exist, return -1
  // if the file need to remove is a dir, only remove it when it's empty
  // . and .. cannot be remove, so check name set by iopen_parent
  // remove a file just need to clean the dirent points to it and set its inode's del
  // the real remove will be done at iclose, after everyone close it
  //printf("iremove() path = %s\n", path);
  char name[MAX_NAME + 1];
  inode_t* parent = iopen_parent(path, name);
  if(parent==NULL)return -1;
  if(name[0]=='.'||(name[0]=='.'&&name[1]=='.')){
    iclose(parent);
    return -1;
  }
  uint32_t off[1];
  inode_t* inode = ilookup(parent, name, off, TYPE_NONE);
  //printf("iremove() name = %s\n", name);
  if(inode==NULL){
    iclose(parent);
    return -1;
  }
  if(inode->dinode.type == TYPE_DIR){
    if(idirempty(inode) == -1){//not empty
      iclose(inode);
      iclose(parent);
      return -1;
    }
  }
  inode->del = 1;
  dirent_t dirent;
  memset(&dirent, 0, sizeof dirent);
  //printf("iremove() off = %d\n", off[0]);
  iwrite(parent, *off, &dirent, sizeof dirent);

  //iclose(inode);

  return 0;
}

#endif
