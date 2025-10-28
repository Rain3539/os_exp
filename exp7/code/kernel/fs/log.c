// 简单的日志文件系统实现
//
// 日志提供崩溃后的原子性：
// 所有写入操作都作为一个事务，要么全部发生，要么都不发生。
//
// 日志驻留在超级块之后的已知固定位置。
// 它由头块和一系列更新的块副本组成。
// 头块包含扇区号数组和每个日志块的计数。
//
// 典型用法：
//   begin_op();
//   ...
//   bp = bread(...);
//   bp->data[...] = ...;
//   log_write(bp);
//   ...
//   end_op();

#include "../def.h"
#include "log.h"
#include "bio.h"
#include "fs.h"

#define MAXOPBLOCKS  10  // 单次FS调用可写入的最大磁盘块数
#define LOGSIZE      30  // 日志中的最大块数

int simulate_crash_after_commit = 0;
// 日志状态
struct logstate {
  int start;       // 日志的第一个块号
  int size;        // 日志中的块数
  int outstanding; // 开始但尚未提交的FS系统调用数量
  int committing;  // 在commit()中，请等待
  int dev;
  struct logheader lh;
} log;

static void recover_from_log(void);
static void commit(void);

// 初始化日志
void initlog(int dev, struct superblock *sb_ptr) {
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: logheader太大");

  // 使用传入的超级块指针
  log.start = sb_ptr->logstart;
  log.size = sb_ptr->nlog;
  log.dev = dev;
  
  printf("日志系统初始化: start=%d, size=%d\n", log.start, log.size);
  
  recover_from_log();
}

// 将已提交的块从日志复制到它们的主位置
static void install_trans(int recovering) {
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *lbuf = bread(log.dev, log.start+tail+1); // 读日志块
    struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // 读目标块
    
    // 复制数据
    for(int i = 0; i < BSIZE; i++) {
      dbuf->data[i] = lbuf->data[i];
    }
    
    bwrite(dbuf);  // 写到磁盘
    
    if(!recovering)
      bunpin(dbuf);
    
    brelse(lbuf);
    brelse(dbuf);
  }
}

// 读取日志头从磁盘到内存中的log.lh
static void read_head(void) {
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  log.lh.n = lh->n;
  for (i = 0; i < log.lh.n; i++) {
    log.lh.block[i] = lh->block[i];
  }
  brelse(buf);
}

// 将内存中的log.lh写入磁盘
// 这是提交点，崩溃后日志将恢复
static void write_head(void) {
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = log.lh.n;
  for (i = 0; i < log.lh.n; i++) {
    hb->block[i] = log.lh.block[i];
  }
  bwrite(buf);
  brelse(buf);
}

// 从日志恢复
static void recover_from_log(void) {
  read_head();
  install_trans(1); // 如果已提交，从日志复制
  log.lh.n = 0;
  write_head(); // 清除日志
  
  printf("文件系统日志恢复完成\n");
}

// 开始一个文件系统操作
void begin_op(void) {
  while(1){
    if(log.committing){
      // 正在提交，等待
      continue;
    } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE){
      // 如果这次操作会使日志太大，等待
      continue;
    } else {
      log.outstanding += 1;
      break;
    }
  }
}

// 结束一个文件系统操作
// 提交是否应该发生在此时？
// 是，如果这是最后一个未完成的操作。
void end_op(void) {
  int do_commit = 0;

  log.outstanding -= 1;
  
  if(log.committing)
    panic("log.committing");
    
  if(log.outstanding == 0){
    do_commit = 1;
    log.committing = 1;
  }

  if(do_commit){
    // 调用commit不持有锁，因为这不允许
    // 在日志提交期间的任何并发操作
    commit();
    log.committing = 0;
  }
}

// 将已修改的块从缓存复制到日志
static void write_log(void) {
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *to = bread(log.dev, log.start+tail+1); // 日志块
    struct buf *from = bread(log.dev, log.lh.block[tail]); // 缓存块
    
    // 复制数据
    for(int i = 0; i < BSIZE; i++) {
      to->data[i] = from->data[i];
    }
    
    bwrite(to);  // 写到日志
    brelse(from);
    brelse(to);
  }
}

// 提交事务
static void commit(void) {
  if (log.lh.n > 0) {
    write_log();     // 将已修改的块从缓存写入日志
    write_head();    // 将头写入磁盘 -- 真正的提交点
    // --- 添加模拟崩溃逻辑 ---
    if(simulate_crash_after_commit) {
      printf("--- 模拟崩溃 (在 Commit 之后, Install 之前) ---\n");
      // 不调用 install_trans() 也不清除日志
      // 立即返回，模拟掉电
      return;
  }
    install_trans(0); // 现在将写入从日志安装到主位置
    log.lh.n = 0;
    write_head();    // 擦除事务从日志
  }
}

// 调用者已修改b->data并完成它。
// 记录块号并在brelse()时固定在缓存中。
// commit()/write_log()将进行磁盘写入。
//
// log_write()在一个事务期间对块的每次修改
// 替换块在日志中的先前记录。
void log_write(struct buf *b) {
  int i;

  if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
    panic("事务太大");
  if (log.outstanding < 1)
    panic("log_write 在op外部");

  for (i = 0; i < log.lh.n; i++) {
    if (log.lh.block[i] == b->blockno)   // 日志吸收
      break;
  }
  log.lh.block[i] = b->blockno;
  if (i == log.lh.n) {  // 添加新块到日志?
    bpin(b);
    log.lh.n++;
  }
}