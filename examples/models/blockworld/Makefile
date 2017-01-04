SMTTOOLS = ${HOME}/work/projects/SMT/tools
MOSES_BIN = ${SMTTOOLS}/mosesdecoder/bin
MOSES_SCRIPTS = ${SMTTOOLS}/mosesdecoder/scripts
KENLM_BIN = $(SMTTOOLS)/kenlm/bin


blockworld-tatoeba.en.kenlm: ../../data/blocksworld.mono.en ../../data/Tatoeba.en-es.en
	cat $^ | ${KENLMHOME}/bin/lmplz -o 3 > $(@:kenlm=arpa)
	${KENLM_BIN}/build_binary $(@:kenlm=arpa) $@

en-sv/phrase-table.gz:
	${MOSES_SCRIPTS}/training/train-model.perl \
		-corpus ../../data/blocksworld.en-sv \
		-model-dir=$(dir $@) -e sv -f en \
		-phrase-word-alignment \
		-last-step 6
	mv model/$@ $@

sv-en/phrase-table.gz:
	${MOSES_SCRIPTS}/training/train-model.perl \
		-corpus ../../data/blocksworld.en-sv \
		-model-dir=$(dir $@) -e en -f sv \
		-phrase-word-alignment \
		-last-step 6

%/phrase-table.binphr.idx: %/phrase-table.gz
	gzip -cd $< | LC_ALL=C sort |\
	${MOSES_BIN}/processPhraseTable \
		-ttable 0 0 - -nscores 4 -alignment-info -out $(@:.binphr.idx=)

%/phrase-table: %/phrase-table.gz
	${MOSES_BIN}/CreateProbingPT $< $@ 4
