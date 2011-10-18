
(define match-f
  (letrec ((match-f1 (lambda (expr pattern kvmap escape)
            (cond
              ((null? pattern)
               (escape 'no-match))
              ((symbol? pattern)
               (hash-set! kvmap pattern expr))
              ((and (pair? pattern)
                    (pair? expr))
               (match-f1 (car expr) (car pattern) kvmap escape)
               (match-f1 (cdr expr) (cdr pattern) kvmap escape))
              (else
                (escape 'error-pattern))))))
    ;; the actual func
    (lambda (expr pattern)
      (call/ec (lambda (k)
                 (let ((retval (make-hash)))
                   (match-f1 expr
                             pattern
                             retval
                             (lambda (what) (k what)))
                   retval))))))

(define (match-many exprs patterns)
  (cond
    ((and (null? exprs)
          (null? patterns))
     #f)
    ((or (null? exprs)
         (null? patterns))
     (error "length dont match"))
    (else
      (let 

(define pair-match
  (lambda-syntax (expr pattern)
    `(match-f ,expr ',pattern)))

;; Would like to see this:
;; (pair-match expr
;;   [()        empty]
;;   [(a)       one]
;;   [(a b)     two]
;;   [(a . b)   dottwo]
;;   [(a b c)   three]
;;   [(a b . c) onedottwo]
;;   [_         anything]) -> (tag . hash)

(define expr '(lambda () 1 2 3))
(hash-pprint (pair-match expr (IDENT FORMALS . BODY)))

