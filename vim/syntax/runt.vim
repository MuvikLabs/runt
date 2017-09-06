" Quit when a syntax file was already loaded
if exists('b:current_syntax') | finish| endif

syntax region potionString start=/\v"/ skip=/\v\\./ end=/\v"/
syntax match potionComment "\v#.*$" 
syntax match potionNumber "\<[0-9]*\.\?"
syntax match bracket "\[\|\]"

highlight link potionString String
highlight link potionComment comment
highlight link potionNumber Type
highlight link bracket Operator

let b:current_syntax = 'runt'

