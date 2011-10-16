
(define-syntax when
  (syntax-rules ()
    ((when test stmt1 stmt2 ...)
     (if test
       (begin stmt1
              stmt2 ...)))))

(let ((if #t))
  (when if
    (set! if 'now)
    (display if)))

