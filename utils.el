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

(defun my-wrap-region-with-ifdef (macro-name)
  "Surround the current region with #ifdef MACRO-NAME / #endif lines.

The region is expanded to whole lines, and #ifdef/#endif are
inserted as separate lines around it."
  (interactive "sMacro name for #ifdef: ")
  (unless (use-region-p)
    (user-error "No active region"))
  (let* ((start (region-beginning))
         (end (region-end)))
    (save-excursion
      ;; Expand to full lines
      (goto-char start)
      (setq start (line-beginning-position))
      (goto-char end)
      ;; If end is at the beginning of a line, don't include that extra line
      (unless (bolp)
        (forward-line 1))
      (setq end (point))

      ;; Insert #endif after the region first (so start offset stays valid)
      (goto-char end)
      (insert (format "#endif // %s\n" macro-name))

      ;; Insert #ifdef before the region
      (goto-char start)
      (insert (format "#ifdef %s\n" macro-name)))))

(global-set-key (kbd "C-c i") #'my-wrap-region-with-ifdef)
