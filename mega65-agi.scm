(define memories
    '(
        (memory zeroPage (address (#x2 . #x7f)) (type ram) (qualifier zpage)
            (section (registers (#x2 . #x7f)))
            (section zzpage)
        )
        (memory stackPage (address (#x100 . #x1ff)) (type ram)
            (section stack)
        )

        (memory runtime (address (#x200 . #x16ff))
            (scatter-to runtime_startup)
            (section
                code
                data
                switch
                cdata
            )
        )

        (memory unbanked_bss (address (#x1700 . #x1fff)) (section zpsave cstack zdata))
        
        (memory autostart
            (address (#x2001 . #x7fff)) (type any)
            (section
                (programStart #x2001)
                (startup #x200e)
                (section runtime_startup #x2200)
                (section data_init_table)
                (section
                    initstext
                    initsdata
                    initsrodata
                )
                (section disk_driver_autostart)
            )
        )

        (memory disk_driver (address (#x8000 . #x9fff))
            (scatter-to disk_driver_autostart)
            (section
                disktext
                diskdata
                diskrodata
            )
        )
        (memory disk_driver_bss (address (#xa000 . #xbfff))
            (section
                diskbss
            )
        )

        (memory gamecode
            (address (#x8004000 . #x801ffff)) (type any)
            (section
                (gamecode_sprhs #x8004000)
                (gamecode_sprls #x8006000)
                (gamecode_lgclo #x800A000)
                (gamecode_lgchi #x800E000)
                (gamecode_engin #x8012000)
                (gamecode_picdr #x8014000)
                (gamecode_parsr #x8016000)
                (gamecode_gamsv #x8018000)
                (gamecode_guidl #x801A000)
                (gamecode_enghi #x801C000)
            )
        )

        (memory highspeed_sprite
            (address (#x2000 . #x3fff))
            (scatter-to gamecode_sprhs)
            (section hs_spritetext hs_spritedata hs_spriterodata)
        )

        (memory lospeed_sprite
            (address (#x4000 . #x7fff))
            (scatter-to gamecode_sprls)
            (section ls_spritetext ls_spritedata ls_spriterodata)
        )

        (memory gameengine
            (address (#x2000 . #x3fff))
            (scatter-to gamecode_engin)
            (section enginetext enginedata enginerodata)
        )

        (memory logic_low
            (address (#x8000 . #xbfff))
            (scatter-to gamecode_lgclo)
            (section ll_text ll_data ll_rodata)
        )

        (memory logic_high
            (address (#x8000 . #xbfff))
            (scatter-to gamecode_lgchi)
            (section lh_text lh_data lh_rodata)
        )

        (memory banked_bss
            (address (#xc000 . #xcfff))
            (section banked_bss)
        )

        (memory picdraw
            (address (#xe000 . #xfff0))
            (scatter-to gamecode_picdr)
            (section picdraw_text picdraw_data picdraw_rodata)
        )

        (memory parser
            (address (#xe000 . #xfff0))
            (scatter-to gamecode_parsr)
            (section parser_text parser_data parser_rodata)
        )

        (memory gamesave
            (address (#xe000 . #xfff0))
            (scatter-to gamecode_gamsv)
            (section gamesave_text gamesave_data gamesave_rodata)
        )

        (memory gui
            (address (#xe000 . #xfff0))
            (scatter-to gamecode_guidl)
            (section gui_text gui_data gui_rodata)
        )

        (memory engine_high
            (address (#x8000 . #xbfff))
            (scatter-to gamecode_enghi)
            (section eh_text eh_data eh_rodata)
        )

        (memory screenmem0
            (address (#x2d000 . #x2e7ff)) (type bss) (qualifier far)
            (section screenmem0))
        (memory screenmem1
            (address (#x2e800 . #x2ffff)) (type bss) (qualifier far)
            (section screenmem1))
        (memory prioritydata
            (address (#x14000 . #x17fff)) (type bss) (qualifier far)
            (section prioritydata))
        (memory extradata
            (address (#x18000 . #x1f6ff)) (type bss) (qualifier far)
            (section extradata))
        (memory chipram2
            (address (#xff86000 . #xff87fff)) (type bss) (qualifier far)
            (section chipram2))
        (memory colorram
            (address (#xff81000 . #xff827ff)) (type bss) (qualifier far)
            (section colorram))
        (block heap (size #x0))
        (block cstack (size #x800))
    )
)
