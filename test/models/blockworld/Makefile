
DOCENTHOME = ../../..
EXTERNAL = ${DOCENTHOME}/DEBUG/external/Install

SMTTOOLS = ${HOME}/work/projects/SMT/tools
MOSESHOME = ${SMTTOOLS}/mosesdecoder
KENLMHOME = $(SMTTOOLS)/kenlm


blockworld-tatoeba.en.kenlm: ../../data/blocksworld.mono.en ../../data/Tatoeba.en-es.en
	cat $^ | ${KENLMHOME}/bin/lmplz -o 3 > $(@:kenlm=arpa)
	${KENLMHOME}/bin/build_binary $(@:kenlm=arpa) $@

en-sv/phrase-table.gz: Makefile
	${MOSESHOME}/scripts/training/train-model.perl \
		-corpus ../../data/blocksworld.en-sv \
		-phrase-word-alignment \
		-model-dir=$(dir $@) -e sv -f en \
		-last-step 6
	mv model/$@ $@

sv-en/phrase-table.gz: Makefile
	${MOSESHOME}/scripts/training/train-model.perl \
		-corpus ../../data/blocksworld.en-sv \
		-phrase-word-alignment \
		-model-dir=$(dir $@) -e en -f sv \
		-last-step 6


%/phrase-table.binphr.idx: %/phrase-table.gz
	gzip -cd $< | LC_ALL=C sort |\
	${EXTERNAL}/moses_external/bin/processPhraseTable \
		-ttable 0 0 - -nscores 4 -alignment-info -out $(@:.binphr.idx=)

