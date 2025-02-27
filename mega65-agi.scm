(define memories
  '((memory program
            (address (#x2001 . #x9fff)) (type any)
            (section (programStart #x2001) (startup #x200e)))
    (memory zeroPage (address (#x2 . #x7f)) (type ram) (qualifier zpage)
	    (section (registers #x2)))
    (memory stackPage (address (#x100 . #x1ff)) (type ram))
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
        (address (#x18000 . #x1f6ff)) (type bss) (qualifier far)
        (section extradata))
    (memory colorram
        (address (#xff81000 . #xff81bff)) (type bss) (qualifier far)
        (section colorram))
    (block heap (size #x0))
    ))
