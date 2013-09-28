

/* Copyright (c) 2011, Peter Barrett  
**  
** Permission to use, copy, modify, and/or distribute this software for  
** any purpose with or without fee is hereby granted, provided that the  
** above copyright notice and this permission notice appear in all copies.  
** 
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL  
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED  
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR  
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES  
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,  
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS  
** SOFTWARE.  
*/

#include "Platform.h"
#include "USBAPI.h"
#include "USBDesc.h"

#if defined(USBCON)
#ifdef HID_ENABLED

//#define RAWHID_ENABLED

//	Singletons for mouse and keyboard

Mouse_ Mouse;
Keyboard_ Keyboard;

//================================================================================
//================================================================================

//	HID report descriptor

#define LSB(_x) ((_x) & 0xFF)
#define MSB(_x) ((_x) >> 8)

#define RAWHID_USAGE_PAGE	0xFFC0
#define RAWHID_USAGE		0x0C00
#define RAWHID_TX_SIZE 64
#define RAWHID_RX_SIZE 64

extern const u8 _hidReportDescriptor[] PROGMEM;
const u8 _hidReportDescriptor[] = {
	
	//	Mouse
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)	// 54
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x85, 0x01,                    //     REPORT_ID (1)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x09, 0x38,                    //     USAGE (Wheel)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xc0,                          //   END_COLLECTION
    0xc0,                          // END_COLLECTION

	//	Keyboard
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)	// 47
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x02,                    //   REPORT_ID (2)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
   
	0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    
	0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    
	0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    
	0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0,                          // END_COLLECTION

#if RAWHID_ENABLED
	//	RAW HID
	0x06, LSB(RAWHID_USAGE_PAGE), MSB(RAWHID_USAGE_PAGE),	// 30
	0x0A, LSB(RAWHID_USAGE), MSB(RAWHID_USAGE),

	0xA1, 0x01,				// Collection 0x01
    0x85, 0x03,             // REPORT_ID (3)
	0x75, 0x08,				// report size = 8 bits
	0x15, 0x00,				// logical minimum = 0
	0x26, 0xFF, 0x00,		// logical maximum = 255

	0x95, 64,				// report count TX
	0x09, 0x01,				// usage
	0x81, 0x02,				// Input (array)

	0x95, 64,				// report count RX
	0x09, 0x02,				// usage
	0x91, 0x02,				// Output (array)
	0xC0					// end collection
#endif
};

extern const HIDDescriptor _hidInterface PROGMEM;
const HIDDescriptor _hidInterface =
{
	D_INTERFACE(HID_INTERFACE,1,3,0,0),
	D_HIDREPORT(sizeof(_hidReportDescriptor)),
	D_ENDPOINT(USB_ENDPOINT_IN (HID_ENDPOINT_INT),USB_ENDPOINT_TYPE_INTERRUPT,0x40,0x01)
};

//================================================================================
//================================================================================
//	Driver

u8 _hid_protocol = 1;
u8 _hid_idle = 1;

#define WEAK __attribute__ ((weak))

int WEAK HID_GetInterface(u8* interfaceNum)
{
	interfaceNum[0] += 1;	// uses 1
	return USB_SendControl(TRANSFER_PGM,&_hidInterface,sizeof(_hidInterface));
}

int WEAK HID_GetDescriptor(int i)
{
	return USB_SendControl(TRANSFER_PGM,_hidReportDescriptor,sizeof(_hidReportDescriptor));
}

void WEAK HID_SendReport(u8 id, const void* data, int len)
{
	USB_Send(HID_TX, &id, 1);
	USB_Send(HID_TX | TRANSFER_RELEASE,data,len);
}

bool WEAK HID_Setup(Setup& setup)
{
	u8 r = setup.bRequest;
	u8 requestType = setup.bmRequestType;
	if (REQUEST_DEVICETOHOST_CLASS_INTERFACE == requestType)
	{
		if (HID_GET_REPORT == r)
		{
			//HID_GetReport();
			return true;
		}
		if (HID_GET_PROTOCOL == r)
		{
			//Send8(_hid_protocol);	// TODO
			return true;
		}
	}
	
	if (REQUEST_HOSTTODEVICE_CLASS_INTERFACE == requestType)
	{
		if (HID_SET_PROTOCOL == r)
		{
			_hid_protocol = setup.wValueL;
			return true;
		}

		if (HID_SET_IDLE == r)
		{
			_hid_idle = setup.wValueL;
			return true;
		}
	}
	return false;
}

//================================================================================
//================================================================================
//	Mouse

Mouse_::Mouse_(void) : _buttons(0)
{
}

void Mouse_::begin(void) 
{
}

void Mouse_::end(void) 
{
}

void Mouse_::click(uint8_t b)
{
	_buttons = b;
	move(0,0,0);
	_buttons = 0;
	move(0,0,0);
}

void Mouse_::move(signed char x, signed char y, signed char wheel)
{
	u8 m[4];
	m[0] = _buttons;
	m[1] = x;
	m[2] = y;
	m[3] = wheel;
	HID_SendReport(1,m,4);
}

void Mouse_::buttons(uint8_t b)
{
	if (b != _buttons)
	{
		_buttons = b;
		move(0,0,0);
	}
}

void Mouse_::press(uint8_t b) 
{
	buttons(_buttons | b);
}

void Mouse_::release(uint8_t b)
{
	buttons(_buttons & ~b);
}

bool Mouse_::isPressed(uint8_t b)
{
	if ((b & _buttons) > 0) 
		return true;
	return false;
}

//================================================================================
//================================================================================
//	Keyboard

Keyboard_::Keyboard_(void) 
{
}

void Keyboard_::begin(void) 
{
	setKmap(DEFAULT_KMAP);
}

void Keyboard_::begin(uint8_t kmap) 
{
	setKmap(kmap);
}

void Keyboard_::end(void) 
{
}

void Keyboard_::setKmap(uint8_t lang)
{
	_kmap = lang;
}

void Keyboard_::sendReport(KeyReport* keys)
{
	HID_SendReport(2,keys,sizeof(KeyReport));
}

extern
const uint16_t _asciimap[256] PROGMEM;

#define SHIFT		0x0100
#define ALTGR		0x0200
#define DK			0x0400

const uint16_t _asciimap[256] =
{
/* 000 0x000 */	0x00,         0x00,             // NUL
/* 001 0x001 */	0x00,         0x00,             // SOH
/* 002 0x002 */	0x00,         0x00,             // STX
/* 003 0x003 */	0x00,         0x00,             // ETX
/* 004 0x004 */	0x00,         0x00,             // EOT
/* 005 0x005 */	0x00,         0x00,             // ENQ
/* 006 0x006 */	0x00,         0x00,             // ACK  
/* 007 0x007 */	0x00,         0x00,             // BEL
/* 008 0x008 */	0x2a,		  0x2a,			// BS	Backspace
/* 009 0x009 */	0x2b,		  0x2b,			// TAB	Tab
/* 010 0x00A */	0x28,		  0x28,			// LF	Enter
/* 011 0x00B */	0x00,         0x00,             // VT 
/* 012 0x00C */	0x00,         0x00,             // FF 
/* 013 0x00D */	0x00,         0x00,             // CR 
/* 014 0x00E */	0x00,         0x00,             // SO 
/* 015 0x00F */	0x00,         0x00,             // SI 
/* 016 0x010 */	0x00,         0x00,             // DEL
/* 017 0x011 */	0x00,         0x00,             // DC1
/* 018 0x012 */	0x00,         0x00,             // DC2
/* 019 0x013 */	0x00,         0x00,             // DC3
/* 020 0x014 */	0x00,         0x00,             // DC4
/* 021 0x015 */	0x00,         0x00,             // NAK
/* 022 0x016 */	0x00,         0x00,             // SYN
/* 023 0x017 */	0x00,         0x00,             // ETB
/* 024 0x018 */	0x00,         0x00,             // CAN
/* 025 0x019 */	0x00,         0x00,             // EM 
/* 026 0x01A */	0x00,         0x00,             // SUB
/* 027 0x01B */	0x00,         0x00,             // ESC
/* 028 0x01C */	0x00,         0x00,             // FS 
/* 029 0x01D */	0x00,         0x00,             // GS 
/* 030 0x01E */	0x00,         0x00,             // RS 
/* 031 0x01F */	0x00,         0x00,             // US 

/* 032 0x020 */	0x2c,		  0x2c,		     // ' '
/* 033 0x021 */	0x1e|SHIFT,	  0x1e|SHIFT,	 // !
/* 034 0x022 */	0x34|SHIFT,	  0x1f|SHIFT,	 // "
/* 035 0x023 */	0x20|SHIFT,   0x20|ALTGR,    // #
/* 036 0x024 */	0x21|SHIFT,   0x21|SHIFT,    // $
/* 037 0x025 */	0x22|SHIFT,   0x22|SHIFT,    // %
/* 038 0x026 */	0x24|SHIFT,   0x23|SHIFT,    // &
/* 039 0x027 */	0x34,         0x2d,          // '
/* 040 0x028 */	0x26|SHIFT,   0x25|SHIFT,    // (
/* 041 0x029 */	0x27|SHIFT,   0x26|SHIFT,    // )
/* 042 0x02A */	0x25|SHIFT,   0x30|SHIFT,    // *
/* 043 0x02B */	0x2e|SHIFT,   0x30,          // +
/* 044 0x02C */	0x36,         0x36,          // ,
/* 045 0x02D */	0x2d,         0x38,          // - 	//33 = ñ
/* 046 0x02E */	0x37,         0x37,          // .
/* 047 0x02F */	0x38,         0x24|SHIFT,    // /
/* 048 0x030 */	0x27,         0x27,          // 0
/* 049 0x031 */	0x1e,         0x1e,          // 1
/* 050 0x032 */	0x1f,         0x1f,          // 2
/* 051 0x033 */	0x20,         0x20,          // 3
/* 052 0x034 */	0x21,         0x21,          // 4
/* 053 0x035 */	0x22,         0x22,          // 5
/* 054 0x036 */	0x23,         0x23,          // 6
/* 055 0x037 */	0x24,         0x24,          // 7
/* 056 0x038 */	0x25,         0x25,          // 8
/* 057 0x039 */	0x26,         0x26,          // 9
/* 058 0x03A */	0x33|SHIFT,   0x37|SHIFT,      // :
/* 059 0x03B */	0x33,         0x36|SHIFT,      // ;
/* 060 0x03C */	0x36|SHIFT,   0x64,            // <
/* 061 0x03D */	0x2e,         0x27|SHIFT,      // =
/* 062 0x03E */	0x37|SHIFT,   0x64|SHIFT,      // >
/* 063 0x03F */	0x38|SHIFT,   0x2d|SHIFT,      // ?
/* 064 0x040 */	0x1f|SHIFT,   0x1f|ALTGR,      // @
/* 065 0x041 */	0x04|SHIFT,   0x04|SHIFT,      // A
/* 066 0x042 */	0x05|SHIFT,   0x05|SHIFT,      // B
/* 067 0x043 */	0x06|SHIFT,   0x06|SHIFT,      // C
/* 068 0x044 */	0x07|SHIFT,   0x07|SHIFT,      // D
/* 069 0x045 */	0x08|SHIFT,   0x08|SHIFT,      // E
/* 070 0x046 */	0x09|SHIFT,   0x09|SHIFT,      // F
/* 071 0x047 */	0x0a|SHIFT,   0x0a|SHIFT,      // G
/* 072 0x048 */	0x0b|SHIFT,   0x0b|SHIFT,      // H
/* 073 0x049 */	0x0c|SHIFT,   0x0c|SHIFT,      // I
/* 074 0x04A */	0x0d|SHIFT,   0x0d|SHIFT,      // J
/* 075 0x04B */	0x0e|SHIFT,   0x0e|SHIFT,      // K
/* 076 0x04C */	0x0f|SHIFT,   0x0f|SHIFT,      // L
/* 077 0x04D */	0x10|SHIFT,   0x10|SHIFT,      // M
/* 078 0x04E */	0x11|SHIFT,   0x11|SHIFT,      // N
/* 079 0x04F */	0x12|SHIFT,   0x12|SHIFT,      // O
/* 080 0x050 */	0x13|SHIFT,   0x13|SHIFT,      // P
/* 081 0x051 */	0x14|SHIFT,   0x14|SHIFT,      // Q
/* 082 0x052 */	0x15|SHIFT,   0x15|SHIFT,      // R
/* 083 0x053 */	0x16|SHIFT,   0x16|SHIFT,      // S
/* 084 0x054 */	0x17|SHIFT,   0x17|SHIFT,      // T
/* 085 0x055 */	0x18|SHIFT,   0x18|SHIFT,      // U
/* 086 0x056 */	0x19|SHIFT,   0x19|SHIFT,      // V
/* 087 0x057 */	0x1a|SHIFT,   0x1a|SHIFT,      // W
/* 088 0x058 */	0x1b|SHIFT,   0x1b|SHIFT,      // X
/* 089 0x059 */	0x1c|SHIFT,   0x1c|SHIFT,      // Y
/* 090 0x05A */	0x1d|SHIFT,   0x1d|SHIFT,      // Z
/* 091 0x05B */	0x2f,         0x2f|ALTGR,    // [
/* 092 0x05C */	0x31,         0x35|ALTGR,    // bslash
/* 093 0x05D */	0x30,         0x30|ALTGR,    // ]
/* 094 0x05E */	0x23|SHIFT,   0x2f|SHIFT|DK, // ^
/* 095 0x05F */	0x2d|SHIFT,   0x38|SHIFT,    // _
/* 096 0x060 */	0x35,         0x2f|DK,       // `
/* 097 0x061 */	0x04,         0x04,          // a
/* 098 0x062 */	0x05,         0x05,          // b
/* 099 0x063 */	0x06,         0x06,          // c
/* 100 0x064 */	0x07,         0x07,          // d
/* 101 0x065 */	0x08,         0x08,          // e
/* 102 0x066 */	0x09,         0x09,          // f
/* 103 0x067 */	0x0a,         0x0a,          // g
/* 104 0x068 */	0x0b,         0x0b,          // h
/* 105 0x069 */	0x0c,         0x0c,          // i
/* 106 0x06A */	0x0d,         0x0d,          // j
/* 107 0x06B */	0x0e,         0x0e,          // k
/* 108 0x06C */	0x0f,         0x0f,          // l
/* 109 0x06D */	0x10,         0x10,          // m
/* 110 0x06E */	0x11,         0x11,          // n
/* 111 0x06F */	0x12,         0x12,          // o
/* 112 0x070 */	0x13,         0x13,          // p
/* 113 0x071 */	0x14,         0x14,          // q
/* 114 0x072 */	0x15,         0x15,          // r
/* 115 0x073 */	0x16,         0x16,          // s
/* 116 0x074 */	0x17,         0x17,          // t
/* 117 0x075 */	0x18,         0x18,          // u
/* 118 0x076 */	0x19,         0x19,          // v
/* 119 0x077 */	0x1a,         0x1a,          // w
/* 120 0x078 */	0x1b,         0x1b,          // x
/* 121 0x079 */	0x1c,         0x1c,          // y
/* 122 0x07A */	0x1d,         0x1d,          // z
/* 123 0x07B */	0x2f|SHIFT,   0x34|ALTGR,    // { ****
/* 124 0x07C */	0x31|SHIFT,   0x1e|ALTGR,    // |
/* 125 0x07D */	0x30|SHIFT,   0x31|ALTGR,    // }
/* 126 0x07E */	0x35|SHIFT,   0x21|ALTGR,    // ~
/* 127 0x07F */	0,			  0				 // DEL
};

uint8_t USBPutChar(uint8_t c);

// press() adds the specified key (printing, non-printing, or modifier)
// to the persistent key report and sends the report.  Because of the way 
// USB HID works, the host acts like the key remains pressed until we 
// call release(), releaseAll(), or otherwise clear the report and resend.
size_t Keyboard_::press(uint8_t in) 
{
	uint16_t k;
	uint8_t wasDeadKey = 0;
	
	if (in >= 136) {			// it's a non-printing key (not a modifier)
		k = in - 136;
	} else if (in >= 128) {	// it's a modifier key
		_keyReport.modifiers |= (1<<(in-128));
		k = 0;
	} else {				// it's a printing key
		k = pgm_read_word(_asciimap + in * 2 + _kmap);
		if (!k) {
			setWriteError();
			return 0;
		}
		if (k & SHIFT) {
			_keyReport.modifiers |= KEY_MODIFIER_LEFT_SHIFT;
			k &= ~SHIFT;
		}
		if (k & ALTGR) {
			_keyReport.modifiers |= KEY_MODIFIER_RIGHT_ALT;
			k &= ~ALTGR;
		}		
		if (k & DK) {
			k &= ~DK;
			wasDeadKey = 1;
		}
	}
	if (!addToReport(k))
		return 0;
	sendReport(&_keyReport);
	if (wasDeadKey){
		write(' ');
	}
	return 1;
}

uint8_t Keyboard_::addToReport(uint8_t k)
{
	uint8_t i;
	// Add k to the key report only if it's not already present
	// and if there is an empty slot.
    if (_keyReport.keys[0] != k && _keyReport.keys[1] != k && 
		_keyReport.keys[2] != k && _keyReport.keys[3] != k &&
		_keyReport.keys[4] != k && _keyReport.keys[5] != k) {
		
		for (i=0; i<6; i++) {
			if (_keyReport.keys[i] == 0x00) {
				_keyReport.keys[i] = k;
				break;
			}
		}
		if (i == 6) {
			setWriteError();
			return 0;
		}	
	}
	return 1;
}


// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t Keyboard_::release(uint8_t in)
{
	uint16_t k;
	if (in >= 136) {			// it's a non-printing key (not a modifier)
		k = in - 136;
	} else if (in >= 128) {	// it's a modifier key
		_keyReport.modifiers &= ~(1<<(in-128));
		k = 0;
	} else {				// it's a printing key
		k = pgm_read_word(_asciimap + in * 2 + _kmap);
		if (!k) {
			return 0;
		}
		if (k & SHIFT) {
			_keyReport.modifiers &= ~KEY_MODIFIER_LEFT_SHIFT;
			k &= ~SHIFT;
		}
		if (k & ALTGR) {
			_keyReport.modifiers &= ~KEY_MODIFIER_RIGHT_ALT;
			k &= ~ALTGR;
		}		
		if (k & DK) {
			k &= ~DK;
		}
	}
	removeFromReport(k);
	sendReport(&_keyReport);
	return 1;
}

uint8_t Keyboard_::removeFromReport(uint8_t k){

	// Test the key report to see if k is present.  Clear it if it exists.
	// Check all positions in case the key is present more than once (which it shouldn't be)
	for (int i=0; i<6; i++) {
		if (0 != k && _keyReport.keys[i] == k) {
			_keyReport.keys[i] = 0x00;
		}
	}
	return 1;
}

void Keyboard_::releaseAll(void)
{
	_keyReport.keys[0] = 0;
	_keyReport.keys[1] = 0;	
	_keyReport.keys[2] = 0;
	_keyReport.keys[3] = 0;	
	_keyReport.keys[4] = 0;
	_keyReport.keys[5] = 0;	
	_keyReport.modifiers = 0;
	sendReport(&_keyReport);
}

size_t Keyboard_::write(uint8_t c)
{	
	uint8_t p = press(c);		// Keydown
	//uint8_t r = 
	release(c);		// Keyup
	return (p);					// just return the result of press() since release() almost always returns 1
}

#endif

#endif /* if defined(USBCON) */
