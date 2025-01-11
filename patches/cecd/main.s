.3ds
.thumb

.open "code.bin", "build/patched_code.bin", 0x100000

; cecd spr state check
;cec_spr_state_check_addr equ 0x109140

;.org cec_spr_state_check_addr
;  .db 0x00, 0x2F


; final check
;cec_check_addr equ 0x109170
;
;.org cec_check_addr
;  .db 0x21, 0xE0 ; b #0x46



.close