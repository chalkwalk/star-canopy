-- For the man page: a link keeps its words and loses its target, which man
-- cannot follow.
function Link(el)
  return el.content
end
