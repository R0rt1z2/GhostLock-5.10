#include "common.h"

#include <sys/system_properties.h>

struct target_entry {
  const char *label;
  const char *device_match;
  const char *banner_match;
  const char *fp_match;
  struct ksym_config ksym;
};

static const struct target_entry g_targets[] = {
  {
    .label = "Fire OS 8.3.3.8 (RS8338.3339N / 0030132734852)",
    .device_match = "sunstone",
    .banner_match = "-ge7a6aa0ea53f",
    .fp_match = "0030132734852",
    .ksym = {
      .kimage_text_base = 0xffffffc010000000ULL,

      .random_misc_fops_off = 0x022dc390ULL,
      .ashmem_misc_fops_off = 0x02b22070ULL,
      .ashmem_fops_off = 0x0232b2a0ULL,
      .ashmem_ioctl_off = 0x0181f7fcULL,
      .ashmem_compat_ioctl_off = 0x0181f800ULL,
      .ashmem_mmap_off = 0x0181430cULL,
      .ashmem_open_off = 0x0181c5b0ULL,
      .ashmem_release_off = 0x0181c5b4ULL,
      .ashmem_show_fdinfo_off = 0x018143d4ULL,
      .ashmem_read_iter_off = 0x01814280ULL,
      .configfs_read_file_off = 0x0181bda4ULL,
      .configfs_write_bin_file_off = 0x0181bfa8ULL,
      .copy_splice_read_off = 0x01814388ULL,
      .noop_llseek_off = 0x01812a4cULL,

      .init_task_off = 0x0288b440ULL,
      .init_uts_ns_off = 0x0288b1e8ULL,
      .empty_zero_page_off = 0x02c02000ULL,
      .root_task_group_off = 0x02c07d80ULL,
      .selinux_blob_sizes_off = 0x02367880ULL,
      .selinux_enforcing_off = 0x02cd62e0ULL,
      .security_hook_heads_off = 0x023671f0ULL,
      .kmalloc_caches_off = 0x02366d30ULL,
      .anon_pipe_buf_ops_off = 0x021e8b28ULL,

      .slide_random_boot_id_data_off = 0x02ae19c8ULL,
      .slide_sysctl_bootid_off = 0x02cefca4ULL,
      .slide_loggers_0_1_off = 0x02880858ULL,
      .slide_nfulnl_logger_off = 0x02880928ULL,

      .init_cred_off = 0x0289b1f0ULL,
      .panic_scratch_off = 0x02c04f48ULL,
    },
  },
  {
    .label = "Fire OS 8.3.3.7 (RS8337.3186N / 0029461607044)",
    .device_match = "sunstone",
    .banner_match = "-gee3b4a65f51d",
    .fp_match = "0029461607044",
    .ksym = {
      .kimage_text_base = 0xffffffc010000000ULL,

      .random_misc_fops_off = 0x022dc190ULL,
      .ashmem_misc_fops_off = 0x02b22070ULL,
      .ashmem_fops_off = 0x0232b0a0ULL,
      .ashmem_ioctl_off = 0x0181f7f8ULL,
      .ashmem_compat_ioctl_off = 0x0181f7fcULL,
      .ashmem_mmap_off = 0x0181430cULL,
      .ashmem_open_off = 0x0181c5b0ULL,
      .ashmem_release_off = 0x0181c5b4ULL,
      .ashmem_show_fdinfo_off = 0x018143d4ULL,
      .ashmem_read_iter_off = 0x01814280ULL,
      .configfs_read_file_off = 0x0181bda4ULL,
      .configfs_write_bin_file_off = 0x0181bfa8ULL,
      .copy_splice_read_off = 0x01814388ULL,
      .noop_llseek_off = 0x01812a4cULL,

      .init_task_off = 0x0288b440ULL,
      .init_uts_ns_off = 0x0288b1e8ULL,
      .empty_zero_page_off = 0x02c02000ULL,
      .root_task_group_off = 0x02c07d80ULL,
      .selinux_blob_sizes_off = 0x02367680ULL,
      .selinux_enforcing_off = 0x02cd62e0ULL,
      .security_hook_heads_off = 0x02366ff0ULL,
      .kmalloc_caches_off = 0x02366b30ULL,
      .anon_pipe_buf_ops_off = 0x021e8928ULL,

      .slide_random_boot_id_data_off = 0x02ae19c8ULL,
      .slide_sysctl_bootid_off = 0x02cefca4ULL,
      .slide_loggers_0_1_off = 0x02880858ULL,
      .slide_nfulnl_logger_off = 0x02880928ULL,

      .init_cred_off = 0x0289b1f0ULL,
      .panic_scratch_off = 0x02c04f48ULL,
    },
  },
  {
    .label = "Fire OS 8.3.3.6 (RS8336.3103N / 0028656279428)",
    .device_match = "sunstone",
    .banner_match = "-gda010c50409f",
    .fp_match = "0028656279428",
    .ksym = {
      .kimage_text_base = 0xffffffc010000000ULL,

      .random_misc_fops_off = 0x022dc090ULL,
      .ashmem_misc_fops_off = 0x02b22070ULL,
      .ashmem_fops_off = 0x0232afa0ULL,
      .ashmem_ioctl_off = 0x0181f7f8ULL,
      .ashmem_compat_ioctl_off = 0x0181f7fcULL,
      .ashmem_mmap_off = 0x0181430cULL,
      .ashmem_open_off = 0x0181c5b0ULL,
      .ashmem_release_off = 0x0181c5b4ULL,
      .ashmem_show_fdinfo_off = 0x018143d4ULL,
      .ashmem_read_iter_off = 0x01814280ULL,
      .configfs_read_file_off = 0x0181bda4ULL,
      .configfs_write_bin_file_off = 0x0181bfa8ULL,
      .copy_splice_read_off = 0x01814388ULL,
      .noop_llseek_off = 0x01812a4cULL,

      .init_task_off = 0x0288b440ULL,
      .init_uts_ns_off = 0x0288b1e8ULL,
      .empty_zero_page_off = 0x02c02000ULL,
      .root_task_group_off = 0x02c07d80ULL,
      .selinux_blob_sizes_off = 0x02367580ULL,
      .selinux_enforcing_off = 0x02cd62e0ULL,
      .security_hook_heads_off = 0x02366ef0ULL,
      .kmalloc_caches_off = 0x02366a30ULL,
      .anon_pipe_buf_ops_off = 0x021e8828ULL,

      .slide_random_boot_id_data_off = 0x02ae19c8ULL,
      .slide_sysctl_bootid_off = 0x02cefca4ULL,
      .slide_loggers_0_1_off = 0x02880858ULL,
      .slide_nfulnl_logger_off = 0x02880928ULL,

      .init_cred_off = 0x0289b1f0ULL,
      .panic_scratch_off = 0x02c04f48ULL,
    },
  },
  {
    .label = "Fire OS 8.3.3.5 (RS8335.2972N / 0027716721796)",
    .device_match = "sunstone",
    .banner_match = "-g7ec28efd738a",
    .fp_match = "0027716721796",
    .ksym = {
      .kimage_text_base = 0xffffffc010000000ULL,

      .random_misc_fops_off = 0x022dbd50ULL,
      .ashmem_misc_fops_off = 0x02b22070ULL,
      .ashmem_fops_off = 0x0232ac60ULL,
      .ashmem_ioctl_off = 0x0181f7f8ULL,
      .ashmem_compat_ioctl_off = 0x0181f7fcULL,
      .ashmem_mmap_off = 0x0181430cULL,
      .ashmem_open_off = 0x0181c5b0ULL,
      .ashmem_release_off = 0x0181c5b4ULL,
      .ashmem_show_fdinfo_off = 0x018143d4ULL,
      .ashmem_read_iter_off = 0x01814280ULL,
      .configfs_read_file_off = 0x0181bda4ULL,
      .configfs_write_bin_file_off = 0x0181bfa8ULL,
      .copy_splice_read_off = 0x01814388ULL,
      .noop_llseek_off = 0x01812a4cULL,

      .init_task_off = 0x0288b440ULL,
      .init_uts_ns_off = 0x0288b1e8ULL,
      .empty_zero_page_off = 0x02c02000ULL,
      .root_task_group_off = 0x02c07d80ULL,
      .selinux_blob_sizes_off = 0x02367240ULL,
      .selinux_enforcing_off = 0x02cd62e0ULL,
      .security_hook_heads_off = 0x02366bb0ULL,
      .kmalloc_caches_off = 0x023666f0ULL,
      .anon_pipe_buf_ops_off = 0x021e84e8ULL,

      .slide_random_boot_id_data_off = 0x02ae19c8ULL,
      .slide_sysctl_bootid_off = 0x02cefca4ULL,
      .slide_loggers_0_1_off = 0x02880858ULL,
      .slide_nfulnl_logger_off = 0x02880928ULL,

      .init_cred_off = 0x0289b1f0ULL,
      .panic_scratch_off = 0x02c04f48ULL,
    },
  },
  {
    .label = "Fire OS 8.3.3.3 (RS8333.2734N / 0026441592452)",
    .device_match = "sunstone",
    .banner_match = "-g76402f9d634d",
    .fp_match = "0026441592452",
    .ksym = {
      .kimage_text_base = 0xffffffc010000000ULL,

      .random_misc_fops_off = 0x022dc310ULL,
      .ashmem_misc_fops_off = 0x02b22070ULL,
      .ashmem_fops_off = 0x0232b220ULL,
      .ashmem_ioctl_off = 0x0181f7f8ULL,
      .ashmem_compat_ioctl_off = 0x0181f7fcULL,
      .ashmem_mmap_off = 0x0181430cULL,
      .ashmem_open_off = 0x0181c5b0ULL,
      .ashmem_release_off = 0x0181c5b4ULL,
      .ashmem_show_fdinfo_off = 0x018143d4ULL,
      .ashmem_read_iter_off = 0x01814280ULL,
      .configfs_read_file_off = 0x0181bda4ULL,
      .configfs_write_bin_file_off = 0x0181bfa8ULL,
      .copy_splice_read_off = 0x01814388ULL,
      .noop_llseek_off = 0x01812a4cULL,

      .init_task_off = 0x0288b440ULL,
      .init_uts_ns_off = 0x0288b1e8ULL,
      .empty_zero_page_off = 0x02c02000ULL,
      .root_task_group_off = 0x02c07d80ULL,
      .selinux_blob_sizes_off = 0x02367800ULL,
      .selinux_enforcing_off = 0x02cd62e0ULL,
      .security_hook_heads_off = 0x02367170ULL,
      .kmalloc_caches_off = 0x02366cb0ULL,
      .anon_pipe_buf_ops_off = 0x021e8aa8ULL,

      .slide_random_boot_id_data_off = 0x02ae19c8ULL,
      .slide_sysctl_bootid_off = 0x02cefca4ULL,
      .slide_loggers_0_1_off = 0x02880858ULL,
      .slide_nfulnl_logger_off = 0x02880928ULL,

      .init_cred_off = 0x0289b1f0ULL,
      .panic_scratch_off = 0x02c04f48ULL,
    },
  },
  {
    .label = "Fire OS 8.3.3.2/8.3.3.1/8.3.3.0/8.3.2.7 (RS8332.3115N / 0025837710212)",
    .device_match = "sunstone",
    .banner_match = "-gf66ba269681b",
    .fp_match = "0025837710212",
    .ksym = {
      .kimage_text_base = 0xffffffc010000000ULL,

      .random_misc_fops_off = 0x022dbcd0ULL,
      .ashmem_misc_fops_off = 0x02b22070ULL,
      .ashmem_fops_off = 0x0232abe0ULL,
      .ashmem_ioctl_off = 0x0181f7f8ULL,
      .ashmem_compat_ioctl_off = 0x0181f7fcULL,
      .ashmem_mmap_off = 0x0181430cULL,
      .ashmem_open_off = 0x0181c5b0ULL,
      .ashmem_release_off = 0x0181c5b4ULL,
      .ashmem_show_fdinfo_off = 0x018143d4ULL,
      .ashmem_read_iter_off = 0x01814280ULL,
      .configfs_read_file_off = 0x0181bda4ULL,
      .configfs_write_bin_file_off = 0x0181bfa8ULL,
      .copy_splice_read_off = 0x01814388ULL,
      .noop_llseek_off = 0x01812a4cULL,

      .init_task_off = 0x0288b440ULL,
      .init_uts_ns_off = 0x0288b1e8ULL,
      .empty_zero_page_off = 0x02c02000ULL,
      .root_task_group_off = 0x02c07d80ULL,
      .selinux_blob_sizes_off = 0x023671c0ULL,
      .selinux_enforcing_off = 0x02cd62e0ULL,
      .security_hook_heads_off = 0x02366b30ULL,
      .kmalloc_caches_off = 0x02366670ULL,
      .anon_pipe_buf_ops_off = 0x021e8468ULL,

      .slide_random_boot_id_data_off = 0x02ae19c8ULL,
      .slide_sysctl_bootid_off = 0x02cefca4ULL,
      .slide_loggers_0_1_off = 0x02880858ULL,
      .slide_nfulnl_logger_off = 0x02880928ULL,

      .init_cred_off = 0x0289b1f0ULL,
      .panic_scratch_off = 0x02c04f48ULL,
    },
  },
  {
    .label = "Fire OS 8.3.2.4 (RS8324.2314N / 0023153150596)",
    .device_match = "sunstone",
    .banner_match = "-g82c06325e9f8",
    .fp_match = "0023153150596",
    .ksym = {
      .kimage_text_base = 0xffffffc010000000ULL,

      .random_misc_fops_off = 0x022dc290ULL,
      .ashmem_misc_fops_off = 0x02b22070ULL,
      .ashmem_fops_off = 0x0232b1a0ULL,
      .ashmem_ioctl_off = 0x0181f7f8ULL,
      .ashmem_compat_ioctl_off = 0x0181f7fcULL,
      .ashmem_mmap_off = 0x0181430cULL,
      .ashmem_open_off = 0x0181c5b0ULL,
      .ashmem_release_off = 0x0181c5b4ULL,
      .ashmem_show_fdinfo_off = 0x018143d4ULL,
      .ashmem_read_iter_off = 0x01814280ULL,
      .configfs_read_file_off = 0x0181bda4ULL,
      .configfs_write_bin_file_off = 0x0181bfa8ULL,
      .copy_splice_read_off = 0x01814388ULL,
      .noop_llseek_off = 0x01812a4cULL,

      .init_task_off = 0x0288b440ULL,
      .init_uts_ns_off = 0x0288b1e8ULL,
      .empty_zero_page_off = 0x02c01000ULL,
      .root_task_group_off = 0x02c06d80ULL,
      .selinux_blob_sizes_off = 0x02367780ULL,
      .selinux_enforcing_off = 0x02cd52e0ULL,
      .security_hook_heads_off = 0x023670f0ULL,
      .kmalloc_caches_off = 0x02366c30ULL,
      .anon_pipe_buf_ops_off = 0x021e8a28ULL,

      .slide_random_boot_id_data_off = 0x02ae19c8ULL,
      .slide_sysctl_bootid_off = 0x02ceeca4ULL,
      .slide_loggers_0_1_off = 0x02880858ULL,
      .slide_nfulnl_logger_off = 0x02880928ULL,

      .init_cred_off = 0x0289b1f0ULL,
      .panic_scratch_off = 0x02c03f48ULL,
    },
  },
  {
    .label = "Fire OS 8.3.2.2 (RS8322.2053N / 0022045787524)",
    .device_match = "sunstone",
    .banner_match = "-g89f110867b4a",
    .fp_match = "0022045787524",
    .ksym = {
      .kimage_text_base = 0xffffffc010000000ULL,

      .random_misc_fops_off = 0x022dbf90ULL,
      .ashmem_misc_fops_off = 0x02b22070ULL,
      .ashmem_fops_off = 0x0232aea0ULL,
      .ashmem_ioctl_off = 0x0181f7fcULL,
      .ashmem_compat_ioctl_off = 0x0181f800ULL,
      .ashmem_mmap_off = 0x0181430cULL,
      .ashmem_open_off = 0x0181c5b0ULL,
      .ashmem_release_off = 0x0181c5b4ULL,
      .ashmem_show_fdinfo_off = 0x018143d4ULL,
      .ashmem_read_iter_off = 0x01814280ULL,
      .configfs_read_file_off = 0x0181bda4ULL,
      .configfs_write_bin_file_off = 0x0181bfa8ULL,
      .copy_splice_read_off = 0x01814388ULL,
      .noop_llseek_off = 0x01812a4cULL,

      .init_task_off = 0x0288b440ULL,
      .init_uts_ns_off = 0x0288b1e8ULL,
      .empty_zero_page_off = 0x02c01000ULL,
      .root_task_group_off = 0x02c06d80ULL,
      .selinux_blob_sizes_off = 0x02367480ULL,
      .selinux_enforcing_off = 0x02cd52e0ULL,
      .security_hook_heads_off = 0x02366df0ULL,
      .kmalloc_caches_off = 0x02366930ULL,
      .anon_pipe_buf_ops_off = 0x021e8728ULL,

      .slide_random_boot_id_data_off = 0x02ae19c8ULL,
      .slide_sysctl_bootid_off = 0x02ceeca4ULL,
      .slide_loggers_0_1_off = 0x02880858ULL,
      .slide_nfulnl_logger_off = 0x02880928ULL,

      .init_cred_off = 0x0289b1f0ULL,
      .panic_scratch_off = 0x02c03f48ULL,
    },
  },
  {
    .label = "Fire OS 8.3.2.1 (RS8321.1924N / 0021911536772)",
    .device_match = "sunstone",
    .banner_match = "-g096b58f0a5d7",
    .fp_match = "0021911536772",
    .ksym = {
      .kimage_text_base = 0xffffffc010000000ULL,

      .random_misc_fops_off = 0x022dbe90ULL,
      .ashmem_misc_fops_off = 0x02b22070ULL,
      .ashmem_fops_off = 0x0232ada0ULL,
      .ashmem_ioctl_off = 0x0181f7fcULL,
      .ashmem_compat_ioctl_off = 0x0181f800ULL,
      .ashmem_mmap_off = 0x0181430cULL,
      .ashmem_open_off = 0x0181c5b0ULL,
      .ashmem_release_off = 0x0181c5b4ULL,
      .ashmem_show_fdinfo_off = 0x018143d4ULL,
      .ashmem_read_iter_off = 0x01814280ULL,
      .configfs_read_file_off = 0x0181bda4ULL,
      .configfs_write_bin_file_off = 0x0181bfa8ULL,
      .copy_splice_read_off = 0x01814388ULL,
      .noop_llseek_off = 0x01812a4cULL,

      .init_task_off = 0x0288b440ULL,
      .init_uts_ns_off = 0x0288b1e8ULL,
      .empty_zero_page_off = 0x02c01000ULL,
      .root_task_group_off = 0x02c06d80ULL,
      .selinux_blob_sizes_off = 0x02367380ULL,
      .selinux_enforcing_off = 0x02cd52e0ULL,
      .security_hook_heads_off = 0x02366cf0ULL,
      .kmalloc_caches_off = 0x02366830ULL,
      .anon_pipe_buf_ops_off = 0x021e8628ULL,

      .slide_random_boot_id_data_off = 0x02ae19c8ULL,
      .slide_sysctl_bootid_off = 0x02ceeca4ULL,
      .slide_loggers_0_1_off = 0x02880858ULL,
      .slide_nfulnl_logger_off = 0x02880928ULL,

      .init_cred_off = 0x0289b1f0ULL,
      .panic_scratch_off = 0x02c03f48ULL,
    },
  },
  {
    .label = "Fire OS 8.3.2.0 (RS8320.1807N / 0021777289092)",
    .device_match = "sunstone",
    .banner_match = "-g66ce35ca5a96",
    .fp_match = "0021777289092",
    .ksym = {
      .kimage_text_base = 0xffffffc010000000ULL,

      .random_misc_fops_off = 0x022dc050ULL,
      .ashmem_misc_fops_off = 0x02b22070ULL,
      .ashmem_fops_off = 0x0232af60ULL,
      .ashmem_ioctl_off = 0x0181f7f4ULL,
      .ashmem_compat_ioctl_off = 0x0181f7f8ULL,
      .ashmem_mmap_off = 0x01814304ULL,
      .ashmem_open_off = 0x0181c5a8ULL,
      .ashmem_release_off = 0x0181c5acULL,
      .ashmem_show_fdinfo_off = 0x018143ccULL,
      .ashmem_read_iter_off = 0x01814278ULL,
      .configfs_read_file_off = 0x0181bd9cULL,
      .configfs_write_bin_file_off = 0x0181bfa0ULL,
      .copy_splice_read_off = 0x01814380ULL,
      .noop_llseek_off = 0x01812a44ULL,

      .init_task_off = 0x0288b440ULL,
      .init_uts_ns_off = 0x0288b1e8ULL,
      .empty_zero_page_off = 0x02c01000ULL,
      .root_task_group_off = 0x02c06d80ULL,
      .selinux_blob_sizes_off = 0x02367540ULL,
      .selinux_enforcing_off = 0x02cd52e0ULL,
      .security_hook_heads_off = 0x02366eb0ULL,
      .kmalloc_caches_off = 0x023669f0ULL,
      .anon_pipe_buf_ops_off = 0x021e87e8ULL,

      .slide_random_boot_id_data_off = 0x02ae19c8ULL,
      .slide_sysctl_bootid_off = 0x02ceeca4ULL,
      .slide_loggers_0_1_off = 0x02880858ULL,
      .slide_nfulnl_logger_off = 0x02880928ULL,

      .init_cred_off = 0x0289b1f0ULL,
      .panic_scratch_off = 0x02c03f48ULL,
    },
  },
  {
    .label = "Fire OS 8.3.1.9 (RS8319.1664N / 0021508817028)",
    .device_match = "sunstone",
    .banner_match = "-gec87eda1378d",
    .fp_match = "0021508817028",
    .ksym = {
      .kimage_text_base = 0xffffffc010000000ULL,

      .random_misc_fops_off = 0x022dbb50ULL,
      .ashmem_misc_fops_off = 0x02b22058ULL,
      .ashmem_fops_off = 0x0232a958ULL,
      .ashmem_ioctl_off = 0x0181f7f0ULL,
      .ashmem_compat_ioctl_off = 0x0181f7f4ULL,
      .ashmem_mmap_off = 0x01814300ULL,
      .ashmem_open_off = 0x0181c5a4ULL,
      .ashmem_release_off = 0x0181c5a8ULL,
      .ashmem_show_fdinfo_off = 0x018143c8ULL,
      .ashmem_read_iter_off = 0x01814274ULL,
      .configfs_read_file_off = 0x0181bd98ULL,
      .configfs_write_bin_file_off = 0x0181bf9cULL,
      .copy_splice_read_off = 0x0181437cULL,
      .noop_llseek_off = 0x01812a40ULL,

      .init_task_off = 0x0288b440ULL,
      .init_uts_ns_off = 0x0288b1e8ULL,
      .empty_zero_page_off = 0x02c01000ULL,
      .root_task_group_off = 0x02c06d80ULL,
      .selinux_blob_sizes_off = 0x02366f40ULL,
      .selinux_enforcing_off = 0x02cd52e0ULL,
      .security_hook_heads_off = 0x023668b0ULL,
      .kmalloc_caches_off = 0x023663f0ULL,
      .anon_pipe_buf_ops_off = 0x021e82e8ULL,

      .slide_random_boot_id_data_off = 0x02ae19c8ULL,
      .slide_sysctl_bootid_off = 0x02ceeca4ULL,
      .slide_loggers_0_1_off = 0x02880858ULL,
      .slide_nfulnl_logger_off = 0x02880928ULL,

      .init_cred_off = 0x0289b1f0ULL,
      .panic_scratch_off = 0x02c03f48ULL,
    },
  },
};

#define N_TARGETS (sizeof(g_targets) / sizeof(g_targets[0]))

static void read_proc_version(char *buf, size_t len) {
  buf[0] = 0;
  int fd = open("/proc/version", O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    return;
  }
  ssize_t n = read(fd, buf, len - 1);
  close(fd);
  if (n > 0) {
    buf[n] = 0;
  }
}

static void read_fingerprint(char *buf, size_t len) {
  buf[0] = 0;
  char prop[PROP_VALUE_MAX];
  if (__system_property_get("ro.bootimage.build.fingerprint", prop) > 0) {
    snprintf(buf, len, "%s", prop);
  }
  size_t used = strlen(buf);
  if (__system_property_get("ro.build.fingerprint", prop) > 0) {
    snprintf(buf + used, len - used, "|%s", prop);
  }
}

static int device_ok(const struct target_entry *t, const char *dev) {
  if (!t->device_match) return 1;
  return dev[0] && strcmp(dev, t->device_match) == 0;
}

const char *ksym_config_select_for_device(void) {
  char banner[512];
  char fp[2 * PROP_VALUE_MAX + 2];
  char dev[PROP_VALUE_MAX];
  read_proc_version(banner, sizeof(banner));
  read_fingerprint(fp, sizeof(fp));
  if (__system_property_get("ro.product.device", dev) <= 0) {
    dev[0] = 0;
  }

  for (size_t i = 0; i < N_TARGETS; i++) {
    if (g_targets[i].banner_match && banner[0] &&
        strstr(banner, g_targets[i].banner_match)) {
      if (!device_ok(&g_targets[i], dev)) {
        pr_warning("Target: banner matches %s but device is \"%s\", not \"%s\"; "
                   "refusing\n", g_targets[i].label, dev,
                   g_targets[i].device_match);
        continue;
      }
      g_ksym = g_targets[i].ksym;
      return g_targets[i].label;
    }
  }

  for (size_t i = 0; i < N_TARGETS; i++) {
    if (g_targets[i].fp_match && fp[0] &&
        strstr(fp, g_targets[i].fp_match)) {
      if (!device_ok(&g_targets[i], dev)) {
        continue;
      }
      g_ksym = g_targets[i].ksym;
      pr_warning("Target: matched %s by fingerprint, not kernel banner; "
                 "verify /proc/version if the run fails\n",
                 g_targets[i].label);
      return g_targets[i].label;
    }
  }

  pr_warning("Target: no registry entry matched this build "
             "(banner=\"%.60s\"); using compiled-in defaults\n", banner);
  return NULL;
}
