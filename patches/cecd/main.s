.3ds
.thumb

.open "code.bin", "build/patched_code.bin", 0x100000

; Path that any message will be uploaded via spr, not just the ones destined
; for everyone
.org 0x119ac0
  mov r1, 3

.close