(defun comment-out-all-printf-calls ()
  "Comment out every printf(...) call in the current C/C++ buffer.
Handles multiline printf calls."
  (interactive)
  (save-excursion
    (goto-char (point-min))
    (while (re-search-forward "\\_<printf\\_>[[:space:]\n\r]*(" nil t)
      (let ((start (match-beginning 0)))
        ;; Move to the opening paren.
        (goto-char (1- (point)))
        (condition-case nil
            (progn
              ;; Jump to matching ')'.
              (forward-sexp)
              ;; Include a trailing semicolon if present.
              (skip-chars-forward " \t\n\r")
              (when (looking-at ";")
                (forward-char 1))
              (comment-region start (point)))
          (scan-error
           ;; Unbalanced parentheses; continue searching.
           (goto-char start)
           (forward-char 1)))))))