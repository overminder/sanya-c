
;; - No inheritance yet?
;; - First primitive-class object or just a syntatic form?
;;
;; (primitive-class Animal
;;   [!init (lambda (self name)
;;               (setattr self name name))]
;;   [shout (lambda (self)
;;            (display (getattr self name))
;;            (newline))])
;;
;; (define ani (Animal))
;; ((getattr ani shout))

;; instanitiate the given primitive-class with the given args, a helper procedure
(define (new-primitive-object kls init-args)
  (let* ((attrs (make-hash))
         (obj (vector kls attrs)))
    ;; bindings are created at getattr time.
    (apply (getattr obj !init) init-args)
    obj))

;; create a primitive-class with the given name and slots, or error if the
;; primitive-class with the same name exists.
(define (create-primitive-class name slot-pairs)
  (let* ((kls-attrs (make-hash))
         (kls (vector name kls-attrs))) ; Currently.
    ;; adding default methods
    (hash-set! kls-attrs '!init (lambda (self) #f))

    ;; unpacking attributes
    (for-each
      (lambda (slot)
        (let ((slot-name (car slot))
              (slot-value (cadr slot)))
          (hash-set! kls-attrs slot-name slot-value)))
      slot-pairs)

    ;; The primitive-class object.
    kls))

;; May hook the object's !getattr
(define (getattr1 object slot-name)
  (let ((result
    (let ((attrs (vector-ref object 1))
          (kls-attrs (vector-ref (vector-ref object 0)
                                  1)))
      (cond
        ;; __dict__ hook
        ((eq? slot-name '__dict__)
         attrs)
        ((hash-exists? attrs slot-name)
         (hash-ref attrs slot-name))
        ((hash-exists? kls-attrs slot-name)
         (hash-ref kls-attrs slot-name))
        ;; call !getattr hook
        ((hash-exists? kls-attrs '!getattr)
         ((hash-ref kls-attrs '!getattr) object slot-name))
        (else
          (error "no such attribute"))))))
    (if (procedure? result)
      (lambda args
        (apply result (cons object args)))
      result)))

(define getattr
  (lambda-syntax (object slot-name)
    `(getattr1 ,object ',slot-name)))

;; May hook the object's !setattr
(define (setattr1 object slot-name value)
  (let ((attrs (vector-ref object 1))
        (kls-attrs (vector-ref (vector-ref object 0)
                                1)))
    (cond
      ;; call !setattr hook
      ((hash-exists? kls-attrs '!setattr)
       ((hash-ref kls-attrs '!setattr) object slot-name value))
      (else (hash-set! attrs slot-name value)))))

(define setattr
  (lambda-syntax (object slot-name value)
    `(setattr1 ,object ',slot-name ,value)))

;; May hook the object's !hasattr
(define (hasattr1 object slot-name)
  (let ((attrs (vector-ref object 1))
        (kls-attrs (vector-ref (vector-ref object 0)
                                1)))
    (cond
      ;; call !setattr hook
      ((hash-exists? kls-attrs '!hasattr)
       ((hash-ref kls-attrs '!hasattr) object slot-name))
      (else (hash-exists? attrs slot-name)))))

(define hasattr
  (lambda-syntax (object slot-name)
    `(hasattr1 ,object ',slot-name)))

(define primitive-class
  (lambda-syntax (name . slot-pairs)
    (let* ((slot-names (map car slot-pairs))
           (slot-values (map cadr slot-pairs)))
      `(define ,name ((lambda ,slot-names
          (let ((kls (create-primitive-class ',name (zip ',slot-names
                                               (list ,@slot-names)))))
            ; callable constructor
            (lambda args (new-primitive-object kls args))))
        ,@slot-values)))))

;; not finished..
(primitive-class Class
  [!init
    (lambda (self name bases attrs)
      (setattr self name name)
      (setattr self bases bases)
      (setattr self attrs attrs))]

  [new
    (lambda (self . args)
      )])

(primitive-class Object
  [!init
    (lambda (self)
      )])


(if testing
  (begin
    (primitive-class Animal
      [!init (lambda (self)
                  (display "Inside Animal's constructor.\n"))]
      [!repr (lambda (self)
                       "<Animal object>")]
      [!getattr (lambda (self key)
                     (display "!getattr called -- ")
                     (display key)
                     (newline))])
    (define ani (Animal))
    (pretty-print ((getattr ani !repr)))
    (setattr ani name 'someone)
    (pretty-print (getattr ani name))
    (pretty-print (getattr ani nexist))))

