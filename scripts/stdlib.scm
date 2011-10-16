
(define (@dict-hash key siz)
  (hash key siz))

(define *dict-minimum-size* 8)

(define (make-dict)
  (cons
    (cons 0 *dict-minimum-size*)
    (make-vector *dict-minimum-size* '())))

(define (@dict-nb-items dic)
  (caar dic))

(define (@dict-set-nb-items! dic new-nb)
  (set-car! (car dic) new-nb))

(define (@dict-size dic)
  (cdar dic))

(define (@dict-set-size! dic new-size)
  (set-cdr! (car dic) new-size))

(define (@dict-vec dic)
  (cdr dic))

(define (@dict-add dic key)
  (let* ((vec (@dict-vec dic))
         (siz (@dict-size dic))
         (buc (@dict-hash key siz))
         (lis (vector-ref vec buc))
         (entry (cons key '())))
    (vector-set! vec buc (cons entry lis))
    entry))

(define (@dict-lookup dic key)
  (let* ((vec (@dict-vec dic))
         (siz (@dict-size dic))
         (buc (@dict-hash key siz))
         (lis (vector-ref vec buc)))
    (@lis-lookup lis key)))

(define (@lis-lookup lis key)
  (if (null? lis) #f
    (let ((entry (car lis)))
      (if (eq? (car entry) key) entry
        (@lis-lookup (cdr lis) key)))))

(define (dict-get dic key default)
  (let ((entry (@dict-lookup dic key)))
    (if entry (cdr entry)
      default)))

(define (dict-set! dic key val)
  (let ((entry (@dict-lookup dic key)))
    (if entry (set-cdr! entry val)
      (let ((entry (@dict-add dic key)))
        (set-cdr! entry val)))))

(define (test-dict)
  (define d (make-dict))
  (display "test get fail => ")
  (display (dict-get d 'key 'cannot-get))
  (newline)

  (display "set key => val")
  (dict-set! d 'key 'val)
  (newline)

  (display "get key => (expecting val) ")
  (display (dict-get d 'key 'wtf))
  (newline))

(test-dict)
