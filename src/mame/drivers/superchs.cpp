// license:BSD-3-Clause
// copyright-holders:Bryan McPhail, David Graves
// thanks-to:Richard Bush
/****************************************************************************

    Super Chase                         (c) 1992 Taito

    Driver by Bryan McPhail & David Graves.

    Board Info:

        CPU board:
        M68EC020RP25
        MC68000P12F (stamped 16 MHz)
        TC0570SPC (Taito custom)
        TC0470LIN (Taito custom)
        TC0510NIO (Taito custom)
        TC0480SCP (Taito custom)
        TC0650FDA (Taito custom)
        ADC0809CCN
        93C46 EEPROM

        X2=26.686MHz
        X1=40MHz
        X3=32MHz

        Sound board:
        MC68000P12F (stamped 16 MHz)
        MC68681P
        MB8421 (x2)
        MB87078
        Ensoniq 5510
        Ensoniq 5505

        OSC1=16MHz
        OSC2=30.47618MHz

    (Acknowledgments and thanks to Richard Bush and the Raine team
    for their preliminary Super Chase driver.)

***************************************************************************/

#include "emu.h"
#include "includes/superchs.h"
#include "audio/taito_en.h"
#include "machine/taitoio.h"

#include "cpu/m68000/m68000.h"
#include "machine/adc0808.h"
#include "machine/eepromser.h"
#include "sound/es5506.h"
#include "screen.h"

#include "superchs.lh"


/*********************************************************************/

READ16_MEMBER(superchs_state::shared_ram_r)
{
	if ((offset&1)==0) return (m_shared_ram[offset/2]&0xffff0000)>>16;
	return (m_shared_ram[offset/2]&0x0000ffff);
}

WRITE16_MEMBER(superchs_state::shared_ram_w)
{
	if ((offset&1)==0) {
		if (ACCESSING_BITS_8_15)
			m_shared_ram[offset/2]=(m_shared_ram[offset/2]&0x00ffffff)|((data&0xff00)<<16);
		if (ACCESSING_BITS_0_7)
			m_shared_ram[offset/2]=(m_shared_ram[offset/2]&0xff00ffff)|((data&0x00ff)<<16);
	} else {
		if (ACCESSING_BITS_8_15)
			m_shared_ram[offset/2]=(m_shared_ram[offset/2]&0xffff00ff)|((data&0xff00)<< 0);
		if (ACCESSING_BITS_0_7)
			m_shared_ram[offset/2]=(m_shared_ram[offset/2]&0xffffff00)|((data&0x00ff)<< 0);
	}
}

WRITE32_MEMBER(superchs_state::cpua_ctrl_w)
{
	/*
	CPUA writes 0x00, 22, 72, f2 in that order.
	f2 seems to be the standard in-game value.
	..x...x.
	.xxx..x.
	xxxx..x.
	is there an irq enable in the top nibble?
	*/

	if (ACCESSING_BITS_8_15)
	{
		m_subcpu->set_input_line(INPUT_LINE_RESET, (data &0x200) ? CLEAR_LINE : ASSERT_LINE);
	}

	if (ACCESSING_BITS_0_7)
	{
		/* Lamp control bits of some sort in the lsb */
	}
}

WRITE8_MEMBER(superchs_state::coin_word_w)
{
	machine().bookkeeping().coin_lockout_w(0,~data & 0x01);
	machine().bookkeeping().coin_lockout_w(1,~data & 0x02);
	machine().bookkeeping().coin_counter_w(0, data & 0x04);
	machine().bookkeeping().coin_counter_w(1, data & 0x08);
}

READ8_MEMBER( superchs_state::volume_r )
{
	return ((m_volume->read() * 255) / 100) ^ 0xff; // 00 = full, ff = silent
}


/***********************************************************
             MEMORY STRUCTURES
***********************************************************/

void superchs_state::superchs_map(address_map &map)
{
	map(0x000000, 0x0fffff).rom();
	map(0x100000, 0x11ffff).ram().share("ram");
	map(0x140000, 0x141fff).ram().share("spriteram");
	map(0x180000, 0x18ffff).rw(m_tc0480scp, FUNC(tc0480scp_device::long_r), FUNC(tc0480scp_device::long_w));
	map(0x1b0000, 0x1b002f).rw(m_tc0480scp, FUNC(tc0480scp_device::ctrl_long_r), FUNC(tc0480scp_device::ctrl_long_w));
	map(0x200000, 0x20ffff).ram().share("shared_ram");
	map(0x240000, 0x240003).w(this, FUNC(superchs_state::cpua_ctrl_w));
	map(0x280000, 0x287fff).ram().w(m_palette, FUNC(palette_device::write32)).share("palette");
	map(0x2c0000, 0x2c07ff).rw("taito_en:dpram", FUNC(mb8421_device::left_r), FUNC(mb8421_device::left_w));
	map(0x300000, 0x300007).rw("tc0510nio", FUNC(tc0510nio_device::read), FUNC(tc0510nio_device::write));
	map(0x340000, 0x340007).rw("adc", FUNC(adc0808_device::data_r), FUNC(adc0808_device::address_offset_start_w)).umask32(0xffffffff);
}

void superchs_state::superchs_cpub_map(address_map &map)
{
	map(0x000000, 0x03ffff).rom();
	map(0x200000, 0x20ffff).ram();
	map(0x600000, 0x60ffff).w(m_tc0480scp, FUNC(tc0480scp_device::word_w)); /* Only written upon errors */
	map(0x800000, 0x80ffff).rw(this, FUNC(superchs_state::shared_ram_r), FUNC(superchs_state::shared_ram_w));
	map(0xa00000, 0xa001ff).ram(); /* Extra road control?? */
}

void superchs_state::chase3_cpub_map(address_map &map)
{
	map(0x000000, 0x03ffff).rom();
	map(0x200000, 0x20ffff).ram();
	map(0x400000, 0x40ffff).ram();
	map(0x600000, 0x60ffff).w(m_tc0480scp, FUNC(tc0480scp_device::word_w)); /* Only written upon errors */
	map(0x800000, 0x80ffff).rw(this, FUNC(superchs_state::shared_ram_r), FUNC(superchs_state::shared_ram_w));
	map(0xa00000, 0xa001ff).ram(); /* Extra road control?? */
}

/***********************************************************/

static INPUT_PORTS_START( superchs )
	PORT_START("COINS")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Freeze Screen" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_SERVICE_NO_TOGGLE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 )

	PORT_START("SWITCHES")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_SERVICE2 ) PORT_NAME("Seat Center")   /* seat center (cockpit only) */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON3 ) PORT_NAME("Nitro")
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON4 ) PORT_NAME("Shifter") PORT_TOGGLE
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_NAME("Brake Switch")   /* upright doesn't have brake? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START1 )

	PORT_START("WHEEL")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_REVERSE PORT_NAME("Steering Wheel")

	PORT_START("ACCEL")
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_SENSITIVITY(100) PORT_KEYDELTA(15) PORT_REVERSE PORT_NAME("Gas Pedal")    /* in upright cab, it is a digital (1 bit) switch instead */

	PORT_START("SOUND")
	PORT_ADJUSTER( 75, "PCB - Sound Volume" )
INPUT_PORTS_END

/***********************************************************
                GFX DECODING
**********************************************************/

static const gfx_layout tile16x16_layout =
{
	16,16,  /* 16*16 sprites */
	RGN_FRAC(1,1),
	4,  /* 4 bits per pixel */
	{ 0, 8, 16, 24 },
	{ 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*64, 1*64,  2*64,  3*64,  4*64,  5*64,  6*64,  7*64,
		8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	64*16   /* every sprite takes 128 consecutive bytes */
};

static const gfx_layout charlayout =
{
	16,16,    /* 16*16 characters */
	RGN_FRAC(1,1),
	4,        /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 5*4, 4*4, 3*4, 2*4, 7*4, 6*4, 9*4, 8*4, 13*4, 12*4, 11*4, 10*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8     /* every sprite takes 128 consecutive bytes */
};

static GFXDECODE_START( superchs )
	GFXDECODE_ENTRY( "gfx2", 0x0, tile16x16_layout,  0, 512 )
	GFXDECODE_ENTRY( "gfx1", 0x0, charlayout,        0, 512 )
GFXDECODE_END


/***********************************************************
                 MACHINE DRIVERS
***********************************************************/

MACHINE_CONFIG_START(superchs_state::superchs)

	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu", M68EC020, XTAL(40'000'000)/2) /* 20MHz - verified */
	MCFG_DEVICE_PROGRAM_MAP(superchs_map)
	MCFG_DEVICE_VBLANK_INT_DRIVER("screen", superchs_state,  irq2_line_hold)

	MCFG_DEVICE_ADD("sub", M68000, XTAL(32'000'000)/2) /* 16MHz - verified */
	MCFG_DEVICE_PROGRAM_MAP(superchs_cpub_map)
	MCFG_DEVICE_VBLANK_INT_DRIVER("screen", superchs_state,  irq4_line_hold)

	MCFG_QUANTUM_TIME(attotime::from_hz(480)) /* Need to interleave CPU 1 & 3 */

	MCFG_EEPROM_SERIAL_93C46_ADD("eeprom")

	MCFG_DEVICE_ADD("adc", ADC0809, 500000) // unknown clock
	MCFG_ADC0808_EOC_FF_CB(INPUTLINE("maincpu", 3))
	MCFG_ADC0808_IN0_CB(IOPORT("WHEEL"))
	MCFG_ADC0808_IN1_CB(IOPORT("ACCEL"))
	MCFG_ADC0808_IN2_CB(READ8(*this, superchs_state, volume_r))

	MCFG_DEVICE_ADD("tc0510nio", TC0510NIO, 0)
	MCFG_TC0510NIO_READ_1_CB(IOPORT("COINS"))
	MCFG_TC0510NIO_READ_2_CB(IOPORT("SWITCHES"))
	MCFG_TC0510NIO_READ_3_CB(READLINE("eeprom", eeprom_serial_93cxx_device, do_read)) MCFG_DEVCB_BIT(7)
	MCFG_TC0510NIO_WRITE_3_CB(WRITELINE("eeprom", eeprom_serial_93cxx_device, clk_write)) MCFG_DEVCB_BIT(5)
	MCFG_DEVCB_CHAIN_OUTPUT(WRITELINE("eeprom", eeprom_serial_93cxx_device, di_write)) MCFG_DEVCB_BIT(6)
	MCFG_DEVCB_CHAIN_OUTPUT(WRITELINE("eeprom", eeprom_serial_93cxx_device, cs_write)) MCFG_DEVCB_BIT(4)
	MCFG_TC0510NIO_WRITE_4_CB(WRITE8(*this, superchs_state, coin_word_w))
	// there are 'vibration' control bits somewhere!

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(40*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 40*8-1, 2*8, 32*8-1)
	MCFG_SCREEN_UPDATE_DRIVER(superchs_state, screen_update_superchs)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_GFXDECODE_ADD("gfxdecode", "palette", superchs)
	MCFG_PALETTE_ADD("palette", 8192)
	MCFG_PALETTE_FORMAT(XRGB)

	MCFG_DEVICE_ADD("tc0480scp", TC0480SCP, 0)
	MCFG_TC0480SCP_GFX_REGION(1)
	MCFG_TC0480SCP_TX_REGION(2)
	MCFG_TC0480SCP_OFFSETS(0x20, 0x08)
	MCFG_TC0480SCP_OFFSETS_TX(-1, 0)
	MCFG_TC0480SCP_GFXDECODE("gfxdecode")

	/* sound hardware */
	MCFG_DEVICE_ADD("taito_en", TAITO_EN, 0)
MACHINE_CONFIG_END

MACHINE_CONFIG_START(superchs_state::chase3)
	superchs(config);

	MCFG_DEVICE_MODIFY("sub")
	MCFG_DEVICE_PROGRAM_MAP(chase3_cpub_map)
	MCFG_DEVICE_VBLANK_INT_DRIVER("screen", superchs_state,  irq4_line_hold)
MACHINE_CONFIG_END

/***************************************************************************/

ROM_START( superchs )
	ROM_REGION( 0x100000, "maincpu", 0 )    /* 1024K for 68020 code (CPU A) */
	ROM_LOAD32_BYTE( "d46-35+.ic27", 0x00000, 0x40000, CRC(1575c9a7) SHA1(e3441d6018ed3315c62c5e5c4534d8712b025ae2) ) /* Actually labeled D46 35* */
	ROM_LOAD32_BYTE( "d46-34+.ic25", 0x00001, 0x40000, CRC(c72a4d2b) SHA1(6ef64de15e52007406ce3255071a1f856e0e8b49) ) /* Actually labeled D46 34* */
	ROM_LOAD32_BYTE( "d46-33+.ic23", 0x00002, 0x40000, CRC(3094bcd0) SHA1(b6779b81a3ebec440a9359868dc43fc3a631ee11) ) /* Actually labeled D46 33* */
	ROM_LOAD32_BYTE( "d46-32+.ic21", 0x00003, 0x40000, CRC(4fbeb335) SHA1(430cb753f3a12ab0412e82aef99e6e93b83050d6) ) /* Actually labeled D46 32* */

	ROM_REGION( 0x140000, "taito_en:audiocpu", 0 )   /* Sound cpu */
	ROM_LOAD16_BYTE( "d46-37.ic8", 0x100000, 0x20000, CRC(60b51b91) SHA1(0d0b017808e0a3bdabe8ef5a726bbe16428db06b) )
	ROM_LOAD16_BYTE( "d46-36.ic7", 0x100001, 0x20000, CRC(8f7aa276) SHA1(b3e330e33099d3cbf4cdc43063119b041e9eea3a) )

	ROM_REGION( 0x40000, "sub", 0 ) /* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "d46-24.ic127", 0x00000, 0x20000, CRC(a006baa1) SHA1(e691ddab6cb79444bd6c3fc870e0dff3051d8cf9) )
	ROM_LOAD16_BYTE( "d46-23.ic112", 0x00001, 0x20000, CRC(9a69dbd0) SHA1(13eca492f1db834c599656750864e7003514f3d4) )

	ROM_REGION( 0x200000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "d46-05.ic87", 0x00000, 0x100000, CRC(150d0e4c) SHA1(9240b32900be733b8f44868ed5d64f5f1aaadb47) )   /* SCR 16x16 tiles */
	ROM_LOAD16_BYTE( "d46-06.ic88", 0x00001, 0x100000, CRC(321308be) SHA1(17e724cce39b1331650c1f08d693d057dcd43a3f) )

	ROM_REGION( 0x800000, "gfx2", 0 )
	ROM_LOAD32_BYTE( "d46-01.ic64", 0x000003, 0x200000, CRC(5c2ae92d) SHA1(bee2caed4729a27fa0569d952d6d12170c2aa2a8) )  /* OBJ 16x16 tiles: each rom has 1 bitplane */
	ROM_LOAD32_BYTE( "d46-02.ic65", 0x000002, 0x200000, CRC(a83ca82e) SHA1(03759be87a8d62c0044e8a44e90c47308e32d3e5) )
	ROM_LOAD32_BYTE( "d46-03.ic66", 0x000001, 0x200000, CRC(e0e9cbfd) SHA1(b7deb2c58320af9d1b4273ad2758ce927d2e279c) )
	ROM_LOAD32_BYTE( "d46-04.ic67", 0x000000, 0x200000, CRC(832769a9) SHA1(136ead19edeee90b5be91a6e2f434193dc670fd8) )

	ROM_REGION16_LE( 0x80000, "user1", 0 )
	ROM_LOAD16_WORD( "d46-07.ic34", 0x00000, 0x80000, CRC(c3b8b093) SHA1(f34364248ca7fdaaa1a0f8f6f795f9b4bc935fb9) )    /* STY, used to create big sprites on the fly */

	ROM_REGION16_BE( 0x1000000, "ensoniq.0" , ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "d46-10.ic2", 0xc00000, 0x200000, CRC(306256be) SHA1(e6e5d4a4c0b98470f2aff2e94624dd19af73ec5d) )
	ROM_LOAD16_BYTE( "d46-12.ic4", 0x000000, 0x200000, CRC(a24a53a8) SHA1(5d5fb87a94ceabda89360064d7d9b6d23c4c606b) )
	ROM_RELOAD     (               0x400000, 0x200000 )
	ROM_LOAD16_BYTE( "d46-11.ic5", 0x800000, 0x200000, CRC(d4ea0f56) SHA1(dc8d2ed3c11d0b6f9ebdfde805188884320235e6) )

	ROM_REGION16_BE( 0x80, "eeprom", 0 )
	ROM_LOAD( "eeprom-superchs.bin", 0x0000, 0x0080, CRC(230f0753) SHA1(4c692b35083da71ed866b233c7c9b152a914c95c) )

	ROM_REGION( 0x1400, "plds", 0 )
	ROM_LOAD( "pal16l8bcn-d46-13.ic82", 0x0000, 0x104, CRC(2f32e889) SHA1(a1dccb9f256c5a17133472279839941f49a1d447) )
	ROM_LOAD( "pal16l8bcn-d46-14.ic84", 0x0200, 0x104, CRC(5ac8b5f8) SHA1(1d69673989874eee887f5b4ed119e3e564bf1a5a) )
	ROM_LOAD( "pal16l8bcn-d46-15.ic9",  0x0400, 0x104, CRC(38ea9f36) SHA1(cc314ea9bb76ce6edc0f478ef9cb6d0ade9aa3c0) )
	ROM_LOAD( "palce20v8h-d46-16.ic8",  0x0600, 0x157, CRC(64e1ff9f) SHA1(23a3625ae110cefb53e923232731b63f04ed6432) )
	ROM_LOAD( "palce20v8h-d46-17.ic10", 0x0800, 0x157, CRC(5c9d94e1) SHA1(7759ef2c7b4a57dc0db851bc07c3799939d92c3c) )
	ROM_LOAD( "palce16v8h-d46-18.ic6",  0x0a00, 0x117, CRC(7581b894) SHA1(df2ca10383053c049d6e85dd253ded995e6e6439) )
	ROM_LOAD( "palce16v8h-d46-19.ic7",  0x0c00, 0x117, CRC(a5d863d0) SHA1(0743bcaf2e08467864adbf73402961d0906dd99c) )
	ROM_LOAD( "palce20v8h-d46-20.ic22", 0x0e00, 0x157, CRC(838cbc11) SHA1(ea479a529a06d266bfb3349d0392847563e1df5b) ) /* Located on the sound board */
	ROM_LOAD( "palce20v8h-d46-21.ic23", 0x1000, 0x157, CRC(93c5aac2) SHA1(45d2a071eb4b50965c52046380f53164b6c0d131) ) /* Located on the sound board */
	ROM_LOAD( "palce20v8h-d46-22.ic24", 0x1200, 0x157, CRC(c6a10b06) SHA1(d0b8e79dac0805720b16e1225eb568d7532291ef) ) /* Located on the sound board */
ROM_END

ROM_START( superchsu )
	ROM_REGION( 0x100000, "maincpu", 0 )    /* 1024K for 68020 code (CPU A) */
	ROM_LOAD32_BYTE( "d46-35+.ic27", 0x00000, 0x40000, CRC(1575c9a7) SHA1(e3441d6018ed3315c62c5e5c4534d8712b025ae2) ) /* Actually labeled D46 35* */
	ROM_LOAD32_BYTE( "d46-34+.ic25", 0x00001, 0x40000, CRC(c72a4d2b) SHA1(6ef64de15e52007406ce3255071a1f856e0e8b49) ) /* Actually labeled D46 34* */
	ROM_LOAD32_BYTE( "d46-33+.ic23", 0x00002, 0x40000, CRC(3094bcd0) SHA1(b6779b81a3ebec440a9359868dc43fc3a631ee11) ) /* Actually labeled D46 33* */
	ROM_LOAD32_BYTE( "d46-31+.ic21", 0x00003, 0x40000, CRC(38b983a3) SHA1(c4859cecc2f3506b7090c462cecd3e4eaabe85aa) ) /* Actually labeled D46 31* */

	ROM_REGION( 0x140000, "taito_en:audiocpu", 0 )   /* Sound cpu */
	ROM_LOAD16_BYTE( "d46-37.ic8", 0x100000, 0x20000, CRC(60b51b91) SHA1(0d0b017808e0a3bdabe8ef5a726bbe16428db06b) )
	ROM_LOAD16_BYTE( "d46-36.ic7", 0x100001, 0x20000, CRC(8f7aa276) SHA1(b3e330e33099d3cbf4cdc43063119b041e9eea3a) )

	ROM_REGION( 0x40000, "sub", 0 ) /* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "d46-24.ic127", 0x00000, 0x20000, CRC(a006baa1) SHA1(e691ddab6cb79444bd6c3fc870e0dff3051d8cf9) )
	ROM_LOAD16_BYTE( "d46-23.ic112", 0x00001, 0x20000, CRC(9a69dbd0) SHA1(13eca492f1db834c599656750864e7003514f3d4) )

	ROM_REGION( 0x200000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "d46-05.ic87", 0x00000, 0x100000, CRC(150d0e4c) SHA1(9240b32900be733b8f44868ed5d64f5f1aaadb47) )   /* SCR 16x16 tiles */
	ROM_LOAD16_BYTE( "d46-06.ic88", 0x00001, 0x100000, CRC(321308be) SHA1(17e724cce39b1331650c1f08d693d057dcd43a3f) )

	ROM_REGION( 0x800000, "gfx2", 0 )
	ROM_LOAD32_BYTE( "d46-01.ic64", 0x000003, 0x200000, CRC(5c2ae92d) SHA1(bee2caed4729a27fa0569d952d6d12170c2aa2a8) )  /* OBJ 16x16 tiles: each rom has 1 bitplane */
	ROM_LOAD32_BYTE( "d46-02.ic65", 0x000002, 0x200000, CRC(a83ca82e) SHA1(03759be87a8d62c0044e8a44e90c47308e32d3e5) )
	ROM_LOAD32_BYTE( "d46-03.ic66", 0x000001, 0x200000, CRC(e0e9cbfd) SHA1(b7deb2c58320af9d1b4273ad2758ce927d2e279c) )
	ROM_LOAD32_BYTE( "d46-04.ic67", 0x000000, 0x200000, CRC(832769a9) SHA1(136ead19edeee90b5be91a6e2f434193dc670fd8) )

	ROM_REGION16_LE( 0x80000, "user1", 0 )
	ROM_LOAD16_WORD( "d46-07.ic34", 0x00000, 0x80000, CRC(c3b8b093) SHA1(f34364248ca7fdaaa1a0f8f6f795f9b4bc935fb9) )    /* STY, used to create big sprites on the fly */

	ROM_REGION16_BE( 0x1000000, "ensoniq.0" , ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "d46-10.ic2", 0xc00000, 0x200000, CRC(306256be) SHA1(e6e5d4a4c0b98470f2aff2e94624dd19af73ec5d) )
	ROM_LOAD16_BYTE( "d46-12.ic4", 0x000000, 0x200000, CRC(a24a53a8) SHA1(5d5fb87a94ceabda89360064d7d9b6d23c4c606b) )
	ROM_RELOAD     (               0x400000, 0x200000 )
	ROM_LOAD16_BYTE( "d46-11.ic5", 0x800000, 0x200000, CRC(d4ea0f56) SHA1(dc8d2ed3c11d0b6f9ebdfde805188884320235e6) )

	ROM_REGION16_BE( 0x80, "eeprom", 0 )
	ROM_LOAD( "eeprom-superchs.bin", 0x0000, 0x0080, CRC(230f0753) SHA1(4c692b35083da71ed866b233c7c9b152a914c95c) )

	ROM_REGION( 0x1400, "plds", 0 )
	ROM_LOAD( "pal16l8bcn-d46-13.ic82", 0x0000, 0x104, CRC(2f32e889) SHA1(a1dccb9f256c5a17133472279839941f49a1d447) )
	ROM_LOAD( "pal16l8bcn-d46-14.ic84", 0x0200, 0x104, CRC(5ac8b5f8) SHA1(1d69673989874eee887f5b4ed119e3e564bf1a5a) )
	ROM_LOAD( "pal16l8bcn-d46-15.ic9",  0x0400, 0x104, CRC(38ea9f36) SHA1(cc314ea9bb76ce6edc0f478ef9cb6d0ade9aa3c0) )
	ROM_LOAD( "palce20v8h-d46-16.ic8",  0x0600, 0x157, CRC(64e1ff9f) SHA1(23a3625ae110cefb53e923232731b63f04ed6432) )
	ROM_LOAD( "palce20v8h-d46-17.ic10", 0x0800, 0x157, CRC(5c9d94e1) SHA1(7759ef2c7b4a57dc0db851bc07c3799939d92c3c) )
	ROM_LOAD( "palce16v8h-d46-18.ic6",  0x0a00, 0x117, CRC(7581b894) SHA1(df2ca10383053c049d6e85dd253ded995e6e6439) )
	ROM_LOAD( "palce16v8h-d46-19.ic7",  0x0c00, 0x117, CRC(a5d863d0) SHA1(0743bcaf2e08467864adbf73402961d0906dd99c) )
	ROM_LOAD( "palce20v8h-d46-20.ic22", 0x0e00, 0x157, CRC(838cbc11) SHA1(ea479a529a06d266bfb3349d0392847563e1df5b) ) /* Located on the sound board */
	ROM_LOAD( "palce20v8h-d46-21.ic23", 0x1000, 0x157, CRC(93c5aac2) SHA1(45d2a071eb4b50965c52046380f53164b6c0d131) ) /* Located on the sound board */
	ROM_LOAD( "palce20v8h-d46-22.ic24", 0x1200, 0x157, CRC(c6a10b06) SHA1(d0b8e79dac0805720b16e1225eb568d7532291ef) ) /* Located on the sound board */
ROM_END

ROM_START( superchsj )
	ROM_REGION( 0x100000, "maincpu", 0 )    /* 1024K for 68020 code (CPU A) */
	ROM_LOAD32_BYTE( "d46-28+.ic27", 0x00000, 0x40000, CRC(5c33784f) SHA1(cb3b3bae4fe8f83809c1f437635b3efc1fb4206a) ) /* Actually labeled D46 28* */
	ROM_LOAD32_BYTE( "d46-27+.ic25", 0x00001, 0x40000, CRC(e81125b8) SHA1(a5c9731eb255217861cda0dfad1ee5003f087b81) ) /* Actually labeled D46 27* */
	ROM_LOAD32_BYTE( "d46-26+.ic23", 0x00002, 0x40000, CRC(2aaba1b0) SHA1(13ceaa678bd671c5c88cac35e8a021a180728a69) ) /* Actually labeled D46 26* */
	ROM_LOAD32_BYTE( "d46-25+.ic21", 0x00003, 0x40000, CRC(4241e97a) SHA1(e3e361080e3ebc098805310d41b3afe7f14ff8b4) ) /* Actually labeled D46 25* */

	ROM_REGION( 0x140000, "taito_en:audiocpu", 0 )   /* Sound cpu */
	ROM_LOAD16_BYTE( "d46-30.ic8", 0x100000, 0x20000, CRC(88f8a421) SHA1(4fd0885d398b1b0e127d7462926d1630a635e305) )
	ROM_LOAD16_BYTE( "d46-29.ic7", 0x100001, 0x20000, CRC(04501fa5) SHA1(dfbafc34df8ab0fcaefb5ca4c3143977020b7e58) )

	ROM_REGION( 0x40000, "sub", 0 ) /* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "d46-24.ic127", 0x00000, 0x20000, CRC(a006baa1) SHA1(e691ddab6cb79444bd6c3fc870e0dff3051d8cf9) )
	ROM_LOAD16_BYTE( "d46-23.ic112", 0x00001, 0x20000, CRC(9a69dbd0) SHA1(13eca492f1db834c599656750864e7003514f3d4) )

	ROM_REGION( 0x200000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "d46-05.ic87", 0x00000, 0x100000, CRC(150d0e4c) SHA1(9240b32900be733b8f44868ed5d64f5f1aaadb47) )   /* SCR 16x16 tiles */
	ROM_LOAD16_BYTE( "d46-06.ic88", 0x00001, 0x100000, CRC(321308be) SHA1(17e724cce39b1331650c1f08d693d057dcd43a3f) )

	ROM_REGION( 0x800000, "gfx2", 0 )
	ROM_LOAD32_BYTE( "d46-01.ic64", 0x000003, 0x200000, CRC(5c2ae92d) SHA1(bee2caed4729a27fa0569d952d6d12170c2aa2a8) )  /* OBJ 16x16 tiles: each rom has 1 bitplane */
	ROM_LOAD32_BYTE( "d46-02.ic65", 0x000002, 0x200000, CRC(a83ca82e) SHA1(03759be87a8d62c0044e8a44e90c47308e32d3e5) )
	ROM_LOAD32_BYTE( "d46-03.ic66", 0x000001, 0x200000, CRC(e0e9cbfd) SHA1(b7deb2c58320af9d1b4273ad2758ce927d2e279c) )
	ROM_LOAD32_BYTE( "d46-04.ic67", 0x000000, 0x200000, CRC(832769a9) SHA1(136ead19edeee90b5be91a6e2f434193dc670fd8) )

	ROM_REGION16_LE( 0x80000, "user1", 0 )
	ROM_LOAD16_WORD( "d46-07.ic34", 0x00000, 0x80000, CRC(c3b8b093) SHA1(f34364248ca7fdaaa1a0f8f6f795f9b4bc935fb9) )    /* STY, used to create big sprites on the fly */

	ROM_REGION16_BE( 0x1000000, "ensoniq.0" , ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "d46-10.ic2", 0xc00000, 0x200000, CRC(306256be) SHA1(e6e5d4a4c0b98470f2aff2e94624dd19af73ec5d) )
	ROM_LOAD16_BYTE( "d46-09.ic4", 0x000000, 0x200000, CRC(0acb8bc7) SHA1(62d66925f0eee4cee282c4e0972e08d12acf331c) )
	ROM_RELOAD     (               0x400000, 0x200000 )
	ROM_LOAD16_BYTE( "d46-08.ic5", 0x800000, 0x200000, CRC(4677e820) SHA1(d6427844b08438e45af4c671589a270e46e6dead) )

	ROM_REGION16_BE( 0x80, "eeprom", 0 )
	ROM_LOAD( "eeprom-superchs.bin", 0x0000, 0x0080, CRC(230f0753) SHA1(4c692b35083da71ed866b233c7c9b152a914c95c) )

	ROM_REGION( 0x1400, "plds", 0 )
	ROM_LOAD( "pal16l8bcn-d46-13.ic82", 0x0000, 0x104, CRC(2f32e889) SHA1(a1dccb9f256c5a17133472279839941f49a1d447) )
	ROM_LOAD( "pal16l8bcn-d46-14.ic84", 0x0200, 0x104, CRC(5ac8b5f8) SHA1(1d69673989874eee887f5b4ed119e3e564bf1a5a) )
	ROM_LOAD( "pal16l8bcn-d46-15.ic9",  0x0400, 0x104, CRC(38ea9f36) SHA1(cc314ea9bb76ce6edc0f478ef9cb6d0ade9aa3c0) )
	ROM_LOAD( "palce20v8h-d46-16.ic8",  0x0600, 0x157, CRC(64e1ff9f) SHA1(23a3625ae110cefb53e923232731b63f04ed6432) )
	ROM_LOAD( "palce20v8h-d46-17.ic10", 0x0800, 0x157, CRC(5c9d94e1) SHA1(7759ef2c7b4a57dc0db851bc07c3799939d92c3c) )
	ROM_LOAD( "palce16v8h-d46-18.ic6",  0x0a00, 0x117, CRC(7581b894) SHA1(df2ca10383053c049d6e85dd253ded995e6e6439) )
	ROM_LOAD( "palce16v8h-d46-19.ic7",  0x0c00, 0x117, CRC(a5d863d0) SHA1(0743bcaf2e08467864adbf73402961d0906dd99c) )
	ROM_LOAD( "palce20v8h-d46-20.ic22", 0x0e00, 0x157, CRC(838cbc11) SHA1(ea479a529a06d266bfb3349d0392847563e1df5b) ) /* Located on the sound board */
	ROM_LOAD( "palce20v8h-d46-21.ic23", 0x1000, 0x157, CRC(93c5aac2) SHA1(45d2a071eb4b50965c52046380f53164b6c0d131) ) /* Located on the sound board */
	ROM_LOAD( "palce20v8h-d46-22.ic24", 0x1200, 0x157, CRC(c6a10b06) SHA1(d0b8e79dac0805720b16e1225eb568d7532291ef) ) /* Located on the sound board */
ROM_END

ROM_START( superchsp )
	ROM_REGION( 0x200000, "maincpu", 0 )    /* 2048K for 68020 code (CPU A) */
	ROM_LOAD32_BYTE( "ic21_ffd1.bin", 0x00003, 0x40000, CRC(7a8199ac) SHA1(ab5e9dd34e17ebdbe1b091b9be12b47914164582) )
	ROM_LOAD32_BYTE( "ic23_5935.bin", 0x00002, 0x40000, CRC(2b262660) SHA1(36c772d7dab4a635db9acc7a2cd657a7964ce8e4) )
	ROM_LOAD32_BYTE( "ic25_a56c.bin", 0x00001, 0x40000, CRC(553ebaa9) SHA1(cfa544cb147218c3b3d9be313d83350bed34b348) )
	ROM_LOAD32_BYTE( "ic27_1a46.bin", 0x00000, 0x40000, CRC(544e34c0) SHA1(57039063fa547e650fc66baf132988fb23ba0565) )

	ROM_REGION( 0x140000, "taito_en:audiocpu", 0 )   /* Sound cpu */
	ROM_LOAD16_BYTE( "sound_ic7_lower.bin", 0x100001, 0x20000, CRC(e70902cc) SHA1(ee3d31c4e2c92c4a338d08d379cb80f42f8fa9cf) )
	ROM_LOAD16_BYTE( "sound_ic8_upper.bin", 0x100000, 0x20000, CRC(86eea635) SHA1(49615a152c215e1f940ab16be3b0f1120822969c) )

	ROM_REGION( 0x80000, "sub", 0 ) /* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "ic112_3a05.bin", 0x00001, 0x40000, CRC(f95a477d) SHA1(c3ad1987ecd1f48084fba08687bd75ae804342b3) )
	ROM_LOAD16_BYTE( "ic127_ae27.bin", 0x00000, 0x40000, CRC(8c8cd2a1) SHA1(178ab2df0ea7371ce275d38051643ea19ba88047) )

	ROM_REGION( 0x200000, "gfx1", 0 ) /* SCR 16x16 tiles */
	ROM_LOAD32_BYTE( "0scn.ic9",    0x00000, 0x080000, CRC(d54e80ec) SHA1(83460cf97b0da8523486ede5bd504710c790b1a6) )
	ROM_LOAD32_BYTE( "8scn.ic8",    0x00002, 0x080000, CRC(b3da122d) SHA1(1e4198b2d5ce2144a7ca01f418aca33f799dcad2) )
	ROM_LOAD32_BYTE( "16scn.ic12",  0x00001, 0x080000, CRC(dd26932c) SHA1(31bcc4e0195a6d966829976b89e81e6eb7dde8b6) )
	ROM_LOAD32_BYTE( "24scn.ic13",  0x00003, 0x080000, CRC(4f560680) SHA1(6398013b8fa5aebc905bf31918e990dd7f5d9490) )

	ROM_REGION( 0x800000, "gfx2", 0 )
	ROMX_LOAD( "0lobj.ic14",   0x000003, 0x80000, CRC(972d0866) SHA1(7787312ba99d971eee30d50ddff12629e3bdc8b9) , ROM_SKIP(7) )
	ROMX_LOAD( "16lobj.ic15",  0x000002, 0x80000, CRC(ceefdf0d) SHA1(4463f5d1d81b0fcaa29276c995c31dc89cb97680) , ROM_SKIP(7) )
	ROMX_LOAD( "32lobj.ic16",  0x000001, 0x80000, CRC(1e86a77a) SHA1(886080d3872fe2d592269f6541569a290885a475) , ROM_SKIP(7) )
	ROMX_LOAD( "48lobj.ic17",  0x000000, 0x80000, CRC(1d8f3c72) SHA1(92670fc4d331d1243457b67ec7d98a273d9c9540) , ROM_SKIP(7) )
	ROMX_LOAD( "8lobj.ic4",    0x000007, 0x80000, CRC(e138a3f7) SHA1(94f1deb05286af73f5af60a08944017540ae3387) , ROM_SKIP(7) )
	ROMX_LOAD( "24lobj.ic5",   0x000006, 0x80000, CRC(927e0539) SHA1(8adbd77ae5bf8fa3761ac7c315d1c96f616a0197) , ROM_SKIP(7) )
	ROMX_LOAD( "40lobj.ic6",   0x000005, 0x80000, CRC(3810a203) SHA1(f269c5bea5db18626b716d8c204dace7ab681e28) , ROM_SKIP(7) )
	ROMX_LOAD( "56lobj.ic7",   0x000004, 0x80000, CRC(d66d6b30) SHA1(32e08dacffa706e0b3634fc52cabb2a5fe0f5cf5) , ROM_SKIP(7) )
	ROMX_LOAD( "0h-obj.ic22",  0x400003, 0x80000, CRC(985d31b0) SHA1(8e3c899792de0530f1176af992a90dbbbd3938a8) , ROM_SKIP(7) )
	ROMX_LOAD( "16h-obj.ic23", 0x400002, 0x80000, CRC(1be705e8) SHA1(497ee154ee43e81ee7d7e8106663374be0d5a550) , ROM_SKIP(7) )
	ROMX_LOAD( "32h-obj.ic24", 0x400001, 0x80000, CRC(f9fde123) SHA1(8cdd4c98e2eaca542c9916da1fb933606d3c8e3f) , ROM_SKIP(7) )
	ROMX_LOAD( "48h-obj.ic25", 0x400000, 0x80000, CRC(1c5d28c6) SHA1(9844fd41f7d190b8b11b23e01687f7c40cf8b18a) , ROM_SKIP(7) )
	ROMX_LOAD( "8h-obj.ic30",  0x400007, 0x80000, CRC(5559a1a1) SHA1(cdc2f2f0b086cf2468fb5e2289c2308313be6668) , ROM_SKIP(7) )
	ROMX_LOAD( "24h-obj.ic31", 0x400006, 0x80000, CRC(83e724fb) SHA1(0f93163afa4af328b813502cecaf6824b3717bc7) , ROM_SKIP(7) )
	ROMX_LOAD( "40h-obj.ic32", 0x400005, 0x80000, CRC(6a3b2fa0) SHA1(5a8e7875afe3ba8443811729ede21d23fa7b5082) , ROM_SKIP(7) )
	ROMX_LOAD( "56h-obj.ic33", 0x400004, 0x80000, CRC(3e78a619) SHA1(b37bd46b2098db6c8857ffac7444216c617e4e97) , ROM_SKIP(7) )

	ROM_REGION16_LE( 0x80000, "user1", 0 )
	ROM_LOAD16_BYTE( "0style.ic28", 0x00000, 0x40000, CRC(161263e5) SHA1(3b501dd9c543a9505c3fd7627aa42434eeb1a531) )
	ROM_LOAD16_BYTE( "8style.ic27", 0x00001, 0x40000, CRC(b32f246c) SHA1(be950f0da5d839978961cb77745427ac0bd83a5c) )

	ROM_REGION16_BE(0x800000, "ensoniq.0" , ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("0wave.ic1", 0x000000, 0x080000, CRC(3ffdc22e) SHA1(16cc02895a0219fdecec5da3ce37bb246e511a1f) )
	ROM_LOAD16_BYTE("1wave.ic2", 0x100000, 0x080000, CRC(2ffc7b82) SHA1(e7837753bf4b85fc2973ef4d70afac47a614830c) )
	ROM_LOAD16_BYTE("2wave.ic3", 0x200000, 0x080000, CRC(ab976601) SHA1(2edfceb3bfbc65b61a0f8358b24242c2cf0eebcf) )
	ROM_LOAD16_BYTE("3wave.ic4", 0x300000, 0x080000, CRC(0d4a5994) SHA1(3c44dd47e6598efba844554ff5eefb5264876abb) )
	ROM_LOAD16_BYTE("4wave.ic5", 0x400000, 0x080000, CRC(8d803555) SHA1(e58c3673a1e047f8dede605f44ac29990b41c35e) )
	ROM_LOAD16_BYTE("5wave.ic6", 0x500000, 0x080000, CRC(8e486d83) SHA1(5edad7490596d303686babc930455044dc3c53b9) )
	ROM_LOAD16_BYTE("6wave.ic7", 0x600000, 0x080000, CRC(26312451) SHA1(9f947a11592fd8420fc581914bf16e7ade75390c) )    // -std-
	ROM_LOAD16_BYTE("7wave.ic8", 0x700000, 0x080000, CRC(2edaa9dc) SHA1(72fead505c4f44e5736ff7d545d72dfa37d613e2) )    // -std-

	ROM_REGION16_BE( 0x80, "eeprom", 0 )
	ROM_LOAD( "chase3_defaults.nv", 0x0000, 0x0080, CRC(4b37c69f) SHA1(5c8567441ca12c120c157cb3339165586d4c7ce9 ) )
ROM_END

ROM_START( superchsp2 )
	ROM_REGION( 0x200000, "maincpu", 0 )    /* 2048K for 68020 code (CPU A) */
	ROM_LOAD32_BYTE( "ic21_4355.ic21", 0x00003, 0x40000, CRC(c02a3a6f) SHA1(d7a201f0b2cde1823e215d0f21a27b8d041726bf) )
	ROM_LOAD32_BYTE( "ic23_e80c.ic23", 0x00002, 0x40000, CRC(67741918) SHA1(f8c080e06d56849f86ebdc6b989728a296086d37) )
	ROM_LOAD32_BYTE( "ic25_6806.ic25", 0x00001, 0x40000, CRC(29252bb4) SHA1(e2e86bf0006375abae7709303882a7fed2acbaca) )
	ROM_LOAD32_BYTE( "ic27_306b.ic27", 0x00000, 0x40000, CRC(b1ff55d2) SHA1(6ba560523825f60fc4d75a51f6cb88cde04a2587) )

	ROM_REGION( 0x140000, "taito_en:audiocpu", 0 )   /* Sound cpu */
	ROM_LOAD16_BYTE( "ic7_lower_a960.ic7", 0x100001, 0x20000, CRC(aa77fbbf) SHA1(759dac691e3fd7003798da04c8f8c10ccca0d0c0) )
	ROM_LOAD16_BYTE( "ic8_upper_0040.ic8", 0x100000, 0x20000, CRC(577bd564) SHA1(145ba622e7dd1025a13ab3f110f73f6802dffafb) )

	ROM_REGION( 0x80000, "sub", 0 ) /* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "ic112.ic112", 0x00001, 0x40000, CRC(98b4b89e) SHA1(f28054e76d08aa104bfabeb0eb2d42b9f3487608) )
	ROM_LOAD16_BYTE( "ic127.ic127", 0x00000, 0x40000, CRC(41a7a06f) SHA1(80ffe9167ba490ed3c153dc279df65ee758d8d35) )

	ROM_REGION( 0x200000, "gfx1", 0 ) /* SCR 16x16 tiles */
	ROM_LOAD32_BYTE( "9.ic9",    0x00000, 0x080000, CRC(c0494e23) SHA1(35f94ae1b2f663a7046896f2ae10eacaa8f7c813) )
	ROM_LOAD32_BYTE( "8.ic8",    0x00002, 0x080000, CRC(e668dd6c) SHA1(619278598c89c828846068399cdadb8206284da6) )
	ROM_LOAD32_BYTE( "12.ic12",  0x00001, 0x080000, CRC(b4d20bc2) SHA1(2f9fa38447200a3718e3bc23baceb64e903387dc) )
	ROM_LOAD32_BYTE( "13.ic13",  0x00003, 0x080000, CRC(54090800) SHA1(66aac7896daa843918679b50a6a179ad06921ca1) )

	ROM_REGION( 0x800000, "gfx2", 0 )
	ROMX_LOAD( "14.ic14", 0x000003, 0x80000, CRC(9fbd8237) SHA1(392deb7534a61878e282684a9611f374fd14562a) , ROM_SKIP(7) )
	ROMX_LOAD( "15.ic15", 0x000002, 0x80000, CRC(1665c890) SHA1(aede1554c7f71338654f62bc2699763bc4e400a9) , ROM_SKIP(7) )
	ROMX_LOAD( "16.ic16", 0x000001, 0x80000, CRC(a71fa648) SHA1(95dffff8a20e84a6434627255d89837a70de3a6f) , ROM_SKIP(7) )
	ROMX_LOAD( "17.ic17", 0x000000, 0x80000, CRC(2a33d21b) SHA1(b86a0414c4b2108a06572dac5a9f7b0656c3aa88) , ROM_SKIP(7) )
	ROMX_LOAD( "4.ic4",   0x000007, 0x80000, CRC(5cbd609c) SHA1(11985aaa9ce92ac5170f2ffc330583c9104e3e96) , ROM_SKIP(7) )
	ROMX_LOAD( "5.ic5",   0x000006, 0x80000, CRC(5cd6b545) SHA1(2b176ffa60a680b7e20da430dbc62a700aabceab) , ROM_SKIP(7) )
	ROMX_LOAD( "6.ic6",   0x000005, 0x80000, CRC(4996bc41) SHA1(b5c53bbd64cf603fa11088f681d8d48773f18098) , ROM_SKIP(7) )
	ROMX_LOAD( "7.ic7",   0x000004, 0x80000, CRC(24a96745) SHA1(e9fafade59a44d3dcc8937ca918e9f2e0f00732e) , ROM_SKIP(7) )
	ROMX_LOAD( "22.ic22", 0x400003, 0x80000, CRC(3769316a) SHA1(70e8ba5222ab3f1ba9bb3c71d8e9e737ee01c544) , ROM_SKIP(7) )
	ROMX_LOAD( "23.ic23", 0x400002, 0x80000, CRC(875862d9) SHA1(7e26100ada8d88d6ea5443983c18a0f26fe0dbbc) , ROM_SKIP(7) )
	ROMX_LOAD( "24.ic24", 0x400001, 0x80000, CRC(ebf3ac6b) SHA1(93b887302081d104c36ea6f75a4bf7664ea055a0) , ROM_SKIP(7) )
	ROMX_LOAD( "25.ic25", 0x400000, 0x80000, CRC(053f2717) SHA1(a4f398fcc1a0d82f2782fe885fa52fd470744a1e) , ROM_SKIP(7) )
	ROMX_LOAD( "30.ic30", 0x400007, 0x80000, CRC(f9683857) SHA1(5acab4a6fa55692329a8909323bfd0b4e3e2aeec) , ROM_SKIP(7) )
	ROMX_LOAD( "31.ic31", 0x400006, 0x80000, CRC(67afe831) SHA1(016dc96701d30ae97764b58bc5d5e0011e77557c) , ROM_SKIP(7) )
	ROMX_LOAD( "32.ic32", 0x400005, 0x80000, CRC(1c55ac47) SHA1(2b7cd44cd3fb3cbbd49c4ed717b6e6774f600577) , ROM_SKIP(7) )
	ROMX_LOAD( "33.ic33", 0x400004, 0x80000, CRC(0392a90e) SHA1(abdff64a0db9e3293f8fa50042a8ed9cd29edab6) , ROM_SKIP(7) )

	ROM_REGION16_LE( 0x80000, "user1", 0 )
	ROM_LOAD16_BYTE( "27.ic28", 0x00000, 0x40000, CRC(2686bb57) SHA1(ec793ccd5b1bc40ceda91f8aa913e4a423d1ba0a) )
	ROM_LOAD16_BYTE( "28.ic27", 0x00001, 0x40000, CRC(94bf0bc3) SHA1(9b0d4517e9c3f3a5833fe773b8e2774b9a42f48f) )

	// real ROM labels is unknown
	ROM_REGION16_BE(0x1000000, "ensoniq.0" , ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("0wave.ic1",   0x000000, 0x080000, CRC(3426d5fc) SHA1(e4c846b5e6adc0e0dfc1fc6b1f8867401e859051) )
	ROM_LOAD16_BYTE("1wave.ic2",   0x100000, 0x080000, CRC(86bfd6cb) SHA1(c9d95a079393b911b476e82f6f9319d510d27e31) )
	ROM_LOAD16_BYTE("2wave.ic3",   0x200000, 0x080000, CRC(39ceebab) SHA1(f4bb61b83be8d40762f0d1da69a54778eea11c50) )
	ROM_LOAD16_BYTE("3wave.ic4",   0x300000, 0x080000, CRC(b5731a6a) SHA1(210e4e5e55df05feed1449d58101c9934ef412cb) )
	ROM_LOAD16_BYTE("4wave.ic5",   0x400000, 0x080000, CRC(c3fe791d) SHA1(6e84d337d9317908bc7c4190af922309ad39bb94) )
	ROM_LOAD16_BYTE("5wave.ic6",   0x500000, 0x080000, CRC(be74537f) SHA1(03f1834d22a107d01a628addccb4f143b8bd926e) )
	ROM_LOAD16_BYTE("6wave.ic7",   0x600000, 0x080000, CRC(3ffdc22e) SHA1(16cc02895a0219fdecec5da3ce37bb246e511a1f) )
	ROM_LOAD16_BYTE("7wave.ic8",   0x700000, 0x080000, CRC(2ffc7b82) SHA1(e7837753bf4b85fc2973ef4d70afac47a614830c) )
	ROM_LOAD16_BYTE("8wave.ic11",  0xc00000, 0x080000, CRC(24f6a386) SHA1(4dda91f7f0e58b69d8f0e67deb6539dfc753d87c) )
	ROM_LOAD16_BYTE("9wave.ic12",  0xd00000, 0x080000, CRC(de56906b) SHA1(238fe090c2bafbbbcc5f9f161041abf396bf5f6d) )
	ROM_LOAD16_BYTE("10wave.ic13", 0xe00000, 0x080000, CRC(26312451) SHA1(9f947a11592fd8420fc581914bf16e7ade75390c) )
	ROM_LOAD16_BYTE("11wave.ic14", 0xf00000, 0x080000, CRC(2edaa9dc) SHA1(72fead505c4f44e5736ff7d545d72dfa37d613e2) )

	ROM_REGION16_BE( 0x80, "eeprom", 0 )
	ROM_LOAD( "chase3_defaults.nv", 0x0000, 0x0080, CRC(4b37c69f) SHA1(5c8567441ca12c120c157cb3339165586d4c7ce9 ) )
ROM_END



READ32_MEMBER(superchs_state::main_cycle_r)
{
	if (m_maincpu->pc()==0x702)
		m_maincpu->spin_until_interrupt();

	return m_ram[0];
}

READ16_MEMBER(superchs_state::sub_cycle_r)
{
	if (m_subcpu->pc()==0x454)
		m_subcpu->spin_until_interrupt();

	return m_ram[2]&0xffff;
}

DRIVER_INIT_MEMBER(superchs_state,superchs)
{
	/* Speedup handlers */
	m_maincpu->space(AS_PROGRAM).install_read_handler(0x100000, 0x100003, read32_delegate(FUNC(superchs_state::main_cycle_r),this));
	m_subcpu->space(AS_PROGRAM).install_read_handler(0x80000a, 0x80000b, read16_delegate(FUNC(superchs_state::sub_cycle_r),this));
}

GAMEL( 1992, superchs,         0, superchs, superchs, superchs_state, superchs, ROT0,               "Taito Corporation Japan",   "Super Chase - Criminal Termination (World)", 0, layout_superchs ) // 1993/02/16 11:39:36 SUPER CHASE VER 1.2O
GAMEL( 1992, superchsu, superchs, superchs, superchs, superchs_state, superchs, ROT0,               "Taito America Corporation", "Super Chase - Criminal Termination (US)",    0, layout_superchs ) // 1993/02/16 11:39:36 SUPER CHASE VER 1.2A
GAMEL( 1992, superchsj, superchs, superchs, superchs, superchs_state, superchs, ROT0,               "Taito Corporation",         "Super Chase - Criminal Termination (Japan)", 0, layout_superchs ) // 1993/02/16 11:29:18 SUPER CHASE VER 1.2J
GAMEL( 1992, superchsp, superchs, chase3,   superchs, superchs_state, 0,        ORIENTATION_FLIP_X, "Taito Corporation",         "Super Chase - Criminal Termination (1992/10/26 20:24:29 CHASE 3 VER 1.1, prototype)", 0, layout_superchs ) // has CHASE 3 as the internal description
GAMEL( 1992, superchsp2,superchs, chase3,   superchs, superchs_state, 0,        ORIENTATION_FLIP_X, "Taito Corporation",         "Super Chase - Criminal Termination (1992/01/18 18:29:18 CHASE 3 VER 1.3O, prototype)", 0, layout_superchs )
