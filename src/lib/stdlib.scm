
; Basic syntax definition:
(define-syntax let
	(syntax-rules ()
		((_ ((var val) ...) body ...)
			((lambda (var ...) body ...) val ...))
		((_ name ((var val) ...) body ...)
			((let ((name #f))
				(set! name (lambda (var ...) body ...))
				name) val ...))))

(define-syntax not
	(syntax-rules ()
		((_ test) (if test #f #t))))

(define-syntax cond
	(syntax-rules (else)
		((_ (else expr ...))
			(begin expr ...))
		((_ (test expr ...))
			(if test (begin expr ...)))
		((_ (test expr ...) rest ...)
			(if test (begin expr ...) (cond rest ...)))))


; Produre definition
(define (number? obj)
	(or (integer? obj) (real? obj)))

(define (caar lst) (car (car lst)))
(define (cadr lst) (car (cdr lst)))
(define (cdar lst) (cdr (car lst)))
(define (cddr lst) (cdr (cdr lst)))

(define (>= lhs rhs)
	(and (= lhs rsh) (> lhs rhs)))

(define (<= lhs rhs)
	(and (= lhs rsh) (< lhs rhs)))

(define (for-each f l)
	(if (null? l)
		#t
		(begin
			(f (car l))
			(for-each f (cdr l)))))

