(define memories
  '((memory program
            (address (#x2001 . #x6fff)) (type any)
            (section
                (programStart #x2001)
                (startup #x200e)
            )
    )
    (memory nographics
        (address (#x7000 . #xafff))
        (section nographicstext nographicsdata nographicsrodata)
    )
    (memory nographicsbss
        (address (#xb000 . #xbfff))
        (section nographicsbss)
    )
    (memory zeroPage (address (#x2 . #x7f)) (type ram) (qualifier zpage)
	    (section (registers #x2)))
    (memory stackPage (address (#x100 . #x1ff)) (type ram))
    (memory freeSpace (address (#x1600 . #x1eff)) (section zpsave cstack))
    (memory screenmem0
        (address (#x12000 . #x12bff)) (type bss) (qualifier far)
        (section screenmem0))
    (memory screenmem1
        (address (#x12C00 . #x137ff)) (type bss) (qualifier far)
        (section screenmem1))
    (memory prioritydata
        (address (#x14000 . #x17fff)) (type bss) (qualifier far)
        (section prioritydata))
    (memory extradata
        (address (#x18000 . #x1d6ff)) (type bss) (qualifier far)
        (section extradata))
    (memory chipram2
        (address (#x1d700 . #x1f6ff)) (type bss) (qualifier far)
        (section chipram2))
    (memory colorram
        (address (#xff81000 . #xff81bff)) (type bss) (qualifier far)
        (section colorram))
    (block heap (size #x0))
    (block cstack (size #x800))
    ))
