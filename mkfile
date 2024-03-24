</$objtype/mkfile

TARG=viatemp\
	sha1sum


UPDATE=mkfile\
	${OFILES:%.$O=%.c}\
	${SFILES:%=%.s}\
	${TARG:%=%.c}

$O.sha1sum:		sha1sum.$O sha1-386.$O
	$LD -o $target	$prereq

</sys/src/cmd/mkmany