(declare-project
  :name "pq"
  :author "Andrew Chambers"
  :license "MIT"
  :url "https://github.com/andrewchambers/janet-pq"
  :repo "git+https://github.com/andrewchambers/janet-pq.git")

(defn pkg-config [what]
  (def f (file/popen (string "pkg-config " what)))
  (def v (->>
           (file/read f :all)
           (string/trim)
           (string/split " ")))
  (unless (zero? (file/close f))
    (error "pkg-config failed!"))
  v)

(declare-native
    :name "_pq"
    :cflags (pkg-config "libpq --cflags")
    :lflags (pkg-config "libpq --libs")
    :source ["mod.c"])
