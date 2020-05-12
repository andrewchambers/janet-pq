(declare-project
  :name "pq"
  :author "Andrew Chambers"
  :license "MIT"
  :url "https://github.com/andrewchambers/janet-pq"
  :repo "git+https://github.com/andrewchambers/janet-pq.git")


# XXX We should use the shlex module, or make a pkg-config module once post-deps is in a release.
(def- shlike-grammar ~{
  :ws (set " \t\r\n")
  :escape (* "\\" (capture 1))
  :dq-string (accumulate (* "\"" (any (+ :escape (if-not "\"" (capture 1)))) "\""))
  :sq-string (accumulate (* "'" (any (if-not "'" (capture 1))) "'"))
  :token-char (+ :escape (* (not :ws) (capture 1)))
  :token (accumulate (some :token-char))
  :value (* (any (+ :ws)) (+ :dq-string :sq-string :token) (any :ws))
  :main (any :value)
})

(def- peg (peg/compile shlike-grammar))

(defn shsplit
  "Split a string into 'sh like' tokens, returns
   nil if unable to parse the string."
  [s]
  (peg/match peg s))

(defn pkg-config [what]
  (def f (file/popen (string "pkg-config " what)))
  (def v (->>
           (file/read f :all)
           (string/trim)
           shsplit))
  (unless (zero? (file/close f))
    (error "pkg-config failed!"))
  v)

(declare-source
    :source ["pq.janet"])

(declare-native
    :name "_pq"
    :cflags ["-g" ;(pkg-config "libpq --cflags")]
    :lflags (pkg-config "libpq --libs")
    :source ["pq.c"])
