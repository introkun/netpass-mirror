.3ds
.thumb

.open "code.bin", "build/patched_code.bin", 0x100000


spr_url_addr equ 0x144048

spr_startup_time equ 0x1027be
spr_ap_filter_time equ 0x122968


.org spr_url_addr
//  .asciiz "https://devapi.netpass.cafe/spr"
  .asciiz "https://api.netpass.cafe/spr"

// set spr loop to ~15min
.org spr_startup_time
  .db 0x38

// unset the bssid locking
.org spr_ap_filter_time
  .dw 0

.close
