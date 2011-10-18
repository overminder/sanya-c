
(define match-f
  (letrec ((quoted?
             (lambda (expr)
               (and (pair? expr)
                    (eq? (car expr) 'quote))))
           (should-be-fixnum?
              (lambda (expr)
                (and (pair? expr)
                     (eq? (car expr) '$fixnum))))
           (should-be-symbol?
              (lambda (expr)
                (and (pair? expr)
                     (eq? (car expr) '$symbol))))
           (match-f1 ; tree-recursive
             (lambda (expr pattern kvmap escape)
               (cond
                 ((and (null? pattern)
                       (null? expr))
                  '())

                 ((null? pattern)
                  (escape 'no-match))

                 ((symbol? pattern)
                  (hash-set! kvmap pattern expr))

                 ((quoted? pattern)
                  (if (eq? (cadr pattern) expr) #t
                    (escape 'no-match)))

                 ((should-be-fixnum? pattern)
                  (if (integer? expr) (hash-set! kvmap (cadr pattern) expr)
                    (escape 'no-match)))

                 ((should-be-symbol? pattern)
                  (if (symbol? expr) (hash-set! kvmap (cadr pattern) expr)
                    (escape 'no-match)))

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
                   ;; using a mutable dict to save the results.
                   (match-f1 expr
                             pattern
                             retval
                             (lambda (what) (k what)))
                   retval))))))

(define (match-many expr patterns)
  (cond
    ((null? patterns)
     (error "no match"))
    (else
      (let ((pattern (car patterns))
            (rest-patterns (cdr patterns)))
        (let ((result-hash (match-f expr (car pattern))))
          (if (hash-table? result-hash) (cons (cadr pattern) result-hash)
            (match-many expr rest-patterns)))))))


;; Usage:
;; (pair-match expr
;;   [()        empty]
;;   [(a)       one]
;;   [(a b)     two]
;;   [(a . b)   dottwo]
;;   [(a b c)   three]
;;   [(a b . c) onedottwo]
;;   [_         anything]) -> (tag . hash)
(define pair-match
  (lambda-syntax (expr . pattern-pairs)
    `(match-many ,expr '(,@pattern-pairs))))

;; 'expr, ($symbol name) ($fixnum name) are used as type predicates.
;; or pair construct...
(define (match-scheme-expr expr)
  (pair-match expr
    [()                                       empty-list]
    [('define ($symbol name))                 define-empty-form]
    [('define ($symbol name) expr)            define-form]
    [('define (($symbol name) . formals) . body) define-proc-form]
    [('set! ($symbol name) expr)              set-form]
    [('if pred todo)                          if-form-1]
    [('if pred todo otherwise)                if-form-2]
    [('lambda formals . body)                 lambda-form]
    [('quote datum)                           quoted-form]
    ;; And primitives...
    [(proc . args)                            application-expr]
    [($fixnum value)                          fixnum-expr]
    [($symbol value)                          symbol-expr]
    [_                                        anything]))

(if testing
  (pretty-print (match-scheme-expr '(set! hehe 5))))
