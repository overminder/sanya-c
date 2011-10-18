(load "match.scm")
(load "objsystem.scm")

(primitive-class Walker

  [globals (make-hash)]

  [!init
    (lambda (self)
      (setattr self insn-list (make-vector 1024))
      (setattr self next-insn-id 0)
      (setattr self frame_size 0)
      (setattr self consts (make-vector 128))
      (setattr self next-const-id 0))]

  [add-const
    (lambda (self kval)
      (let ((kid (getattr self next-const-id)))
        (vector-set! (getattr self consts) kid kval)
        (setattr self next-consts-id (+ kid 1))
        kid))]

  [emit
    (lambda (self insn)
      (let ((nid (getattr self next-insn-id)))
        (vector-set! (getattr self insn-list) nid insn)
        (setattr self next-insn-id (+ nid 1))
        nid))]

  [visit
    (lambda (self expr)
      (let ([expr-detail (match-scheme-expr expr)])
        1))])

