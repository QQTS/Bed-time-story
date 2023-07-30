// Monitor(监视器)模块
/*1.功能:
    是为了方便地监控客户计算机的运行状态而引入的.
    它除了负责与GNU/Linux进行交互(例如读入客户程序)之外, 还带有调试器的功能.
*/
// 2.monitor.c 源码解读：
//  程序执行入口：
void init_monitor(int argc, char *argv[])
{
  /* Perform some global initialization. */
  // 步骤一//////////////////////////////////////////////
  /* Parse arguments. */
  /*
  1.把从主程序读入的命令行参数传入parse_args()函数
      1.1 argc:参数个数。
      1.2 argv: 字符指针数组，指向每个参数中第一个字符。
  2.在parse_args()函数内部，定义了一个int o，并且：
    令o = getopt_long(argc, argv, "-bhl:d:p:", table, NULL)
  3.上述getopt_long()函数定义：(在"getopt.h"中)：
    int	getopt_long(int, char * const *, const char *,
    const struct option *, int *)
    对应可以发现，前两个参数，argc以及argv[]又被传入了当前的函数。
    参数三const char *对应于"-bhl:d:p:",短选项字符串。
    参数四const struct option *长选项结构体指出了命令行选项长名称(例如b对应batch),
      选项内是否需要参数,返回值的指针，返回值。
  4.上述getopt_long()函数还有4个全局变量：
    optarg：当前选项对应的参数值。
    optind：下一个将被处理到的参数在argv中的下标值。
    opterr：如果opterr = 0，在getopt、getopt_long、getopt_long_only遇到错误将不会输出错误信息到标准输出流。opterr在非0时，向屏幕输出错误。
    optopt：没有被未标识的选项。
  5.上述getopt_long()函数执行结果：
   （1）如果短选项找到，那么将返回短选项对应的字符。
   （2）如果长选项找到，如果flag为NULL，返回val。如果flag不为空，返回0
   （3）如果遇到一个选项没有在短字符、长字符里面。或者在长字符里面存在二义性的，返回“？”
   （4）如果解析完所有字符没有找到（一般是输入命令参数格式错误，eg： 连斜杠都没有加的选项），返回“-1”
   （5）如果选项需要参数，忘了添加参数。返回值取决于optstring，如果其第一个字符是“：”，则返回“：”，否则返回“？”。
  6.如果getopt_long()函数执行后返回值不是-1:
    根据o取得的不同字符(不同选项)，设置不同的模式，激活不同的功能。
  7.命令行参数解析完毕。本质是设置了相应功能mode=ture.
*/
  parse_args(argc, argv);

  // 步骤二//////////////////////////////////////////////
  /* Open the log file. */
  /*
   1.函数原型：
    void init_log(const char *log_file) {
       if (log_file == NULL) return;
       log_fp = fopen(log_file, "w");
       Assert(log_fp, "Can not open '%s'", log_file);}
   2.接收参数是log_file的指针，如果该文件存在，以可写模式打开；
     若文件不存在，打印错误信息。
  */
  init_log(log_file);

  // 步骤三//////////////////////////////////////////////
  /* Fill the memory with garbage content. */
  /*
    1.函数原型：
      void init_mem() {
        #ifndef DIFF_TEST
          srand(time(0));
          uint32_t *p = (uint32_t *)pmem;
          int i;
          for (i = 0; i < PMEM_SIZE / sizeof(p[0]); i ++) {
          p[i] = rand();
       }
      #endif
      }
    2.函数第三行的stand函数原型：
      void srand(unsigned) __swift_unavailable("Use arc4random instead.");
      功能是设置一个产生伪随机数的起始种子，也就是time(0)当前时间。
    3.函数第四行pmen的原型：
      static uint8_t pmem[PMEM_SIZE] PG_ALIGN = {};
      其中#define PMEM_SIZE (128 * 1024 * 1024)
      =》PMEM_SIZE是内存大小，单位是Byte；
      =》pmem是指向每个内存单元的地址
    4.函数第四行用一个新的指针p来取得内存的初始值，也就是pmen指向的位置；
      用p来遍历每一个内存单元，并向内存写入rand()生成的伪随机数。
      实现了向内存注入garbage。
    */
  init_mem();

  // 步骤四//////////////////////////////////////////////
  /* Perform ISA dependent initialization. */
  /*
      1.init_isa()函数根据ISA的不同有多个版本，以RISCV32为例：
        void init_isa() {
           memcpy(guest_to_host(IMAGE_START), img, sizeof(img));
           restart();}
      2.mencpy函数接受3个参数，分别是：目的地址指针，原地址指针，内存大小。
      3.mencpy中第二个参数是一个数组指针：
        static const uint32_t img [] = {
           0x800002b7,  // lui t0,0x80000
           0x0002a023,  // sw  zero,0(t0)
           0x0002a503,  // lw  a0,0(t0)
           0x0000006b,  // nemu_trap};
      3.mencpy中的第一个参数是：
        void* guest_to_host(paddr_t addr) { return &pmem[addr]; }
        typedef uint32_t paddr_t;
        =》接收了IMAGE_START参数，作为一个32bit的地址，返回一个内存相应指针的地址。
      4.restart()函数原型：
        static void restart() {
          cpu.pc = PMEM_BASE + IMAGE_START;
          cpu.gpr[0]._32 = 0;}
        其中第二行：
             #define IMAGE_START concat(__ISA__, _IMAGE_START)
             #define PMEM_BASE concat(__ISA__, _PMEM_BASE)
             typedef concat(__ISA__, _CPU_state) CPU_state;
             __ISA__会被宏展开成："x86" or "mips32" ...
        =》第二行把pc寄存器的值设置为图片在内存中的物理地址，
           第三行设置了零寄存器=0；
      5.init_isa()函数实现了把文件中定义的img加载到内存，之后
        初始化了pc寄存器和零寄存器。
  */
  init_isa();

  // 步骤五//////////////////////////////////////////////
  /* Load the image to memory. This will overwrite the built-in image. */
  /*
      1.load_img()函数原型：
          static inline long load_img() {
            if (img_file == NULL) {
              Log("No image is given. Use the default build-in image.");
            return 4096; // built-in image size
            }

            FILE *fp = fopen(img_file, "rb");
            Assert(fp, "Can not open '%s'", img_file);

            Log("The image is %s", img_file);

            fseek(fp, 0, SEEK_END);
            long size = ftell(fp);

            fseek(fp, 0, SEEK_SET);
            int ret = fread(guest_to_host(IMAGE_START), size, 1, fp);
            assert(ret == 1);

            fclose(fp);
            return size;
            }
          =》inline代表这个是建议编译器在调用点展开的内联函数。
          =》函数第二行的img_file是全局变量，执行到这里应该已经设置成图片的实际内存地址。
          =》根据img_file指针打开图片文件，‘rb’表示以二进制读取模式打开。
          =》fseek函数把文件指针fp的位置移动到SEEK_END+0的位置，而SEEK_END=2，是EOF文件末尾。
          =》ftell函数返回目前文件指针的位置，作为img文件的大小。
          =》fseek函数把文件指针移动到img文件起始位置。
          =》fread函数读出img文件的内容，完成了img的加载。
  */
  long img_size = load_img();

  // 步骤六//////////////////////////////////////////////
  /*
      1.函数原型：
        void init_regex() {
          int i;
          char error_msg[128];
          int ret;

          for (i = 0; i < NR_REGEX; i ++) {
            ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
            if (ret != 0) {
              regerror(ret, &re[i], error_msg, 128);
            panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
            }
           }
          }
        =》正则表达式编译
  */
  /* Compile the regular expressions. */
  init_regex();

  // 步骤七//////////////////////////////////////////////
  /* Initialize the watchpoint pool. */
  /*
      1.init_wp_pool()函数原型：
        void init_wp_pool() {
          int i;
          for (i = 0; i < NR_WP; i ++) {
          wp_pool[i].NO = i;
          wp_pool[i].next = &wp_pool[i + 1];
          }
          wp_pool[NR_WP - 1].next = NULL;

          head = NULL;
          free_ = wp_pool;
        }
        =》第四行的补充定义
           typedef struct watchpoint {
             int NO;
             struct watchpoint *next;
           } WP;
           static WP wp_pool[NR_WP] = {};
           #define NR_WP 32
           所以wp_pool[NR_WP]本质是一个维护32个watchpoint结构体数组的指针。
           结构体中的NO是作为观测点的编号。
        2.该函数构建了一个32个watchpoint的链表，头指针初始化为NULL，
          free_指针指向头节点。
  */
  init_wp_pool();

  // 步骤八//////////////////////////////////////////////
  /* Initialize differential testing. */
  /*
  1.init_difftest(diff_so_file, img_size, difftest_port)
    函数接受三个参数，
  2.如果第一个参数不为空，则调用dlopen函数，以指定模式打开指定的动态连接库文件，
    并返回一个句柄给调用进程。
    void * dlopen( const char * pathname, int mode );
    dlopen函数描述：
    mode分为：
    RTLD_LAZY 暂缓决定，等有需要时再解出符号
    RTLD_NOW 立即决定，返回前解除所有未决定的符号。
    RTLD_LOCAL
    RTLD_GLOBAL 允许导出符号
    RTLD_GROUP
    RTLD_WORLD
    返回值:如果错误返回NULL，如果成功，返回库引用
  3.用assert语句判断第二步djopen是否成功创建并返回一个句柄。
  4.顺序调用5次dlsym函数：
    void *dlsym(void *handle, const char *symbol);
    函数dlsym()的第一个参数是一个指向已经加载的动态目标的句柄，这个句柄可以是dlopen()函数返回的。
    其中symbol参数是一个以null结尾的符号名。
    返回值是这个符号加载到内存中的地址。如果这个符号 在指定的目标 或者 在由dlopen(3)装载指定的目标时自动装载的其他共享目标中都没有找到，dlsym()返回NULL指针。(dlsym在这些动态目标中执行广度优先搜索)。
    由于符号的值本身可能实际就是NULL，因此，返回的NULL不能直接用来判断是否出错！所以，必须通过dlerror(3)函数以清理掉之前的错误状态，然后调用dlsym()，最后调用dlerror(3)，然后将其返回值保存到一个变量，最后检查是否这个保存的变量值不为NULL。
  5.init_difftest函数在difftest_port端口启动了差异性测试。
  */
  init_difftest(diff_so_file, img_size, difftest_port);

  // 步骤九//////////////////////////////////////////////
  /* Display welcome message. */
  welcome();
}
