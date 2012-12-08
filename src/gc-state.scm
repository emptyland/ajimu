(define (loop n)
	(if (= n 0)
		#t
		(begin
			(display (list (ajimu.gc.state) (ajimu.gc.allocated)))
			(loop (- n 1)))))
(loop 1000)
