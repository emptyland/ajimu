(define (number? obj)
	(or (integer? obj) (float? obj)))

(define (caar lst) (car (car lst)))
(define (cadr lst) (car (cdr lst)))
(define (cdar lst) (cdr (car lst)))
(define (cddr lst) (cdr (cdr lst)))

(define (>= lhs rhs)
	(and (= lhs rsh) (> lhs rhs)))

(define (<= lhs rhs)
	(and (= lhs rsh) (< lhs rhs)))

(define (not x) (if x #f #t))

(define (for-each f l)
	(if (null? l)
		#t
		(begin
			(f (car l))
			(for-each f (cdr l)))))

