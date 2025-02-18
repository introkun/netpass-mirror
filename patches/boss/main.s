.3ds
.thumb

.open "code.bin", "build/patched_code.bin", 0x100000


spr_url_addr equ 0x144048

spr_startup_time equ 0x1027be
spr_ap_filter_time equ 0x122968

trampoline_entry equ 0x10e536

CecdsSprAddSlot equ 0x10f438
getFsUserHandle equ 0x126fa8
FsUserOpenArchive equ 0x126da0
FsUserOpenFile equ 0x118bf0
newFullFileFromHandle equ 0x126d6c
s_handle_fsuser_2 equ 0x14b1a8
FsFileWrite equ 0x118b68
FsFileClose equ 0x118adc
FsUserCloseArchive equ 0x118adc
strlen equ 0x12775c
snprintf equ 0x124b24
CreateFileBuffers equ 0x13d7d8


.org spr_url_addr
;  .asciiz "https://devapi.netpass.cafe/spr"
  .asciiz "https://api.netpass.cafe/spr"

; set spr loop to ~15min
.org spr_startup_time
  .db 0x38

; unset the bssid locking
.org spr_ap_filter_time
  .dw 0

; set the policy list to our own url
.org 0x1074f0
  .asciiz "https://api.netpass.cafe/boss"
.org 0x107514
  .asciiz "%s/policylist/%s/%s/%s?ap=%s"
.org 0x107534
  .asciiz "%s/policylist/%s/%s/%s"

.org trampoline_entry
  bl SaveSlotData

; executable data

CallArg1 equ 0
CallArg2 equ CallArg1 + 0x4
CallArg3 equ CallArg2 + 0x4
FullFilePtr equ CallArg3 + 0x4
FsFilePtr equ FullFilePtr + 0x4
PathArgs equ FsFilePtr + 0x4
SlotBuffer equ PathArgs + 0xC
SlotBufferSize equ SlotBuffer + 0x4
PathBuffer equ SlotBufferSize + 0x4
StackArgsSize equ PathBuffer + 0x34

.org 0x13ade0
.area 0x98
.align 2
SaveSlotData:
  push {r4, r5, lr}
  mov r4, r1 ; buffer
  mov r5, r2 ; size
  ; call the original method
  bl CecdsSprAddSlot
  push {r0, r1, r2, r3}

  sub sp, #StackArgsSize

  str r4, [sp, #SlotBuffer]
  str r5, [sp, #SlotBufferSize]
  ; now we can add our own method here
  ;bl CreateFileBuffers

  ; first we open the sd mmc archive
  bl getFsUserHandle ; user handle is in r0 now
  str r0, [sp, #PathArgs + 8] ; we need in r0 a pointer to the handle
  add r0, sp, #PathArgs + 8
  add r1, sp, #PathArgs ; archive handle, just temporary storage we use
  mov r2, 9 ; SDMC archive
  mov r3, 1 ; empty path type
  mov r4, 0
  str r4, [sp, #CallArg3] ; this will be our empty string
  add r4, sp, #CallArg3
  str r4, [sp, #CallArg1] ; pointer to 0 for path
  mov r4, 1
  str r4, [sp, #CallArg2] ; size=1 for path
  bl FsUserOpenArchive
  cmp r0, #0
  bcc fail1

  ; now we store it into the full handle
  ldr r3, [sp, #PathArgs + 4]
  ldr r2, [sp, #PathArgs]
  ldr r4, [FSUserHandlePtr]
  ldr r1, [r4]
  add r0, sp, #FullFilePtr
  bl newFullFileFromHandle
  cmp r0, #0
  bcc fail1

  ; now we have the full file handle, and thus should be able to open the file we want

  ; build PathArgs
  ; first create the path str
  ldr r0, [sp, SlotBuffer]
  ldr r4, [r0, 0x04] ; size
  str r4, [sp, #CallArg2]
  ldr r4, [r0, 0x30] ; transfer id
  str r4, [sp, #CallArg1]
  ldr r3, [r0, 0x08] ; title id
  ldr r2, [slotPathPatternPtr] ; pattern string
  mov r1, 0x34 ; destination string length
  add r0, sp, PathBuffer ; destination string
  bl snprintf

  add r0, r0, 1 ; add the null character from snprintf
  str r0, [sp, #PathArgs + 8] ; string length
  mov r4, 3 ; ascii path typeSlotPathStr
  str r4, [sp, #PathArgs]
  add r0, sp, PathBuffer ; the newly created string
  str r0, [sp, #PathArgs + 4]

  ; open the file
  mov r3, 0b111 ; open flags
  add r2, sp, #PathArgs
  add r1, sp, #FsFilePtr
  ldr r0, [sp, #FullFilePtr]
  cmp r0, #0
  bcc fail1
  ldr r4, [r0] ; the pointer to the open file method is in the
  ldr r4, [r4] ; first four bytes of FsFullFile
  blx r4

  ; write to the file
  mov r4, 1 ; update
  str r4, [sp, #CallArg3]
  ldr r4, [sp, #SlotBufferSize]
  str r4, [sp, #CallArg2]
  ldr r4, [sp, #SlotBuffer]
  str r4, [sp, #CallArg1]

  bl func_cont
fail1:
  bl fail
.align
FSUserHandlePtr:
  .word s_handle_fsuser_2
slotPathPatternPtr:
  .word slotPathPattern
.endarea

.org 0x10d6c4
.area 0x2C
.db 0, 0 ; zero-termination of "string"
.align 2
func_cont:
  mov r3, 0 ; file offset
  mov r2, 0
  beq fail2
  add r1, sp, PathArgs ; use this as temporary variable again
  ldr r0, [sp, FsFilePtr]
  cmp r0, #0 ; check for null pointer

  ldr r4, [r0] ; the second entry in the LUT at the top is
  add r4, 1*4  ; the file write method
  ldr r4, [r4]
  blx r4
  cmp r0, #0
  bcc fail2

  ; now close up everything
  ldr r0, [sp, FsFilePtr]
  ldr r4, [r0] ; the 12th entry in the LUT at the top is
  add r4, 12*4 ; the file close method
  ldr r4, [r4]

  bl func_cont2
fail2:
  bl fail
.endarea

.org 0x12a948
.area 0x32
.db 0, 0 ; zero-termination of "string"
func_cont2:
  blx r4

  add r0, sp, FullFilePtr
  ldr r0, [r0]
  bl FsUserCloseArchive

fail:
  add sp, #StackArgsSize

  pop {r0, r1, r2, r3}
  pop {r4, r5, pc}


.endarea

; ro data
.org 0x148a7c
slotPathPattern:
  .asciiz "/config/netpass/log_spr/_%08lx_%08lx_%ld"

.close
