(load "lib/stdlib.scm")

(define (fact n)
	(if (= n 1)
		1
		(* n (fact (- n 1)))))

(display (list "fact(14)=" (fact 14)))

(let ((loop '()))
	(set! loop (lambda (i)
		(if (= i 0)
			#t
			(begin
				(display (fact i))
				(loop (- i 1))))))
	(loop 10))
