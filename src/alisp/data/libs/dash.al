
(defun d-map (fn list))
(defun d-map-when (pred rep list))
(defun d-map-first (pred rep list))
(defun d-map-last (pred rep list))
(defun d-map-indexed (fn list))
(defun d-annotate (fn list))
(defun d-splice (pred fun list))
(defun d-splice-list (pred new-list list))
(defun d-mapcat (fn list))
(defun d-copy (arg))

(defun d-filter (pred list))
(defun d-remove (pred list))
(defun d-remove-first (pred list))
(defun d-remove-last (pred list))
(defun d-remove-item (item list))
(defun d-non-nil (list))
(defun d-slice (list from &optional to step))
(defun d-take (n list))
(defun d-take-last (n list))
(defun d-drop (n list))
(defun d-drop-last (n list))
(defun d-take-while (pred list))
(defun d-drop-while (pred list))
(defun d-select-by-indices (indices list))
(defun d-select-columns (columns table))
(defun d-select-column (column table))


(defun d-keep (fn list))
(defun d-concat (&rest lists))
(defun d-flatten (l))
(defun d-flatten-n (num list))
(defun d-replace (old new list))
(defun d-replace-first (old new list))
(defun d-replace-last (old new list))
(defun d-insert-at (n x list))
(defun d-replace-at (n x list))
(defun d-update-at (n func list))
(defun d-remove-at (n list))
(defun d-remove-at-indices (indices list))

(defun d-reduce-from (fn initial-value list))
(defun d-reduce-r-from (fn initial-value list))
(defun d-reduce (fn list))
(defun d-reduce-r (fn list))
(defun d-reductions-from (fn init list))
(defun d-reductions-r-from (fn init list))
(defun d-reductions (fn list))
(defun d-reductions-r (fn list))
(defun d-count (pred list))
(defun d-sum (list))
(defun d-running-sum (list))
(defun d-product (list))
(defun d-running-product (list))
(defun d-inits (list))
(defun d-tails (list))
(defun d-common-prefix (&rest lists))
(defun d-common-suffix (&rest lists))
(defun d-min (list))
(defun d-min-by (comparator list))
(defun d-max (list))
(defun d-max-by (comparator list))


(defun d-iterate (fun init n))
(defun d-unfold (fun seed))

(defun d-any? (pred list))
(defun d-all? (pred list))
(defun d-none? (pred list))
(defun d-only-some? (pred list))
(defun d-contains? (list element))
(defun d-same-items? (list list2))
(defun d-is-prefix? (prefix list))
(defun d-is-suffix? (suffix list))
(defun d-is-infix? (infix list))

(defun d-split-at (n list))
(defun d-split-with (pred list))
(defun d-split-on (item list))
(defun d-split-when (fn list))
(defun d-separate (pred list))
(defun d-partition (n list))
(defun d-partition-all (n list))
(defun d-partition-in-steps (n step list))
(defun d-partition-all-in-steps (n step list))
(defun d-partition-by (fn list))
(defun d-partition-by-header (fn list))
(defun d-partition-after-pred (pred list))
(defun d-partition-before-pred (pred list))
(defun d-partition-before-item (item list))
(defun d-partition-after-item (item list))
(defun d-group-by (fn list))

(defun d-elem-index (elem list))
(defun d-elem-indices (elem list))
(defun d-find-index (pred list))
(defun d-find-last-index (pred list))
(defun d-find-indices (pred list))
(defun d-grade-up (comparator list))
(defun d-grade-down (comparator list))


(defun d-union (list list2))
(defun d-difference (list list2))
(defun d-intersection (list list2))
(defun d-powerset (list))
(defun d-permutations (list))
(defun d-distinct (list))

(defun d-rotate (n list))
(defun d-repeat (n x))
(defun d-cons* (&rest args))
(defun d-snoc (list elem &rest elements))
(defun d-interpose (sep list))
(defun d-interleave (&rest lists))
(defun d-zip-with (fn list1 list2))
(defun d-zip (&rest lists))
(defun d-zip-lists (&rest lists))
(defun d-zip-fill (fill-value &rest lists))
(defun d-unzip (lists))
(defun d-cycle (list))
(defun d-pad (fill-value &rest lists))
(defun d-table (fn &rest lists))
(defun d-table-flat (fn &rest lists))
(defun d-first (pred list))
(defun d-some (pred list))
(defun d-last (pred list))
(defun d-first-item (list))
(defun d-second-item (arg1))
(defun d-third-item (arg1))
(defun d-fourth-item (list))
(defun d-fifth-item (list))
(defun d-last-item (list))
(defun d-butlast (list))
(defun d-sort (comparator list))
(defun d-list (&rest args))
(defun d-fix (fn list))

(defun d-tree-seq (branch children tree))
(defun d-tree-map (fn tree))
(defun d-tree-map-nodes (pred fun tree))
(defun d-tree-reduce (fn tree))
(defun d-tree-reduce-from (fn init-value tree))
(defun d-tree-mapreduce (fn folder tree))
(defun d-tree-mapreduce-from (fn folder init-value tree))
(defun d-clone (list))

(defun d-each (list fn))
(defun d-each-while (list pred fn))
(defun d-each-indexed (list fn))
(defun d-each-r (list fn))
(defun d-each-r-while (list pred fn))
(defun d-dotimes (num fn))
(defun d-doto (eval-initial-value &rest forms))
