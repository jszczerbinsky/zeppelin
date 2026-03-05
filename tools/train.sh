NAME="nnue"

A0=0.5
A1=1.0

B0=0.00
B1=0.33

python run_nnue_trainer.py \
	--name $NAME \
	--l1size 512 \
	--l2size 32 \
	--l3size 32 \
	--sched plateau \
	--plateaupatience 4 \
	--plateaufactor 0.5 \
	--plateaupatience 5 \
	--weightdecay 0.00001 \
	--seed $(echo $RANDOM) \
	--a0 $A0 \
	--a1 $A1 \
	--b0 $B0 \
	--b1 $B1 \
	-e 50
