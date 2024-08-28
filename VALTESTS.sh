make -s -B 

valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -f Test -p pass -u test@comp30023 -n 1 retrieve unimelb-comp30023-2024.cloud.edu.au | diff - out/ret-ed512.out
valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -f Test -p pass -u test@comp30023 -n 2 retrieve unimelb-comp30023-2024.cloud.edu.au | diff - out/ret-mst.out
valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -f Test -u test@comp30023 -p pass1 -n 1 retrieve unimelb-comp30023-2024.cloud.edu.au | diff - out/ret-loginfail.out
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --dsymutil=yes -s ./fetchmail -u test@comp30023 -p pass -n 1 -f Test1 retrieve unimelb-comp30023-2024.cloud.edu.au | diff - out/ret-nofolder.out
valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -n 42 -u test@comp30023 -p pass -f Test retrieve unimelb-comp30023-2024.cloud.edu.au | diff - out/ret-nomessage.out
valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -u test.test@comp30023 -p -p -f Test -n 1 retrieve unimelb-comp30023-2024.cloud.edu.au | diff - out/ret-mst.out
valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -f 'With Space' -n 1 -u test@comp30023 -p pass retrieve unimelb-comp30023-2024.cloud.edu.au | diff - out/ret-mst.out

# valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -f Test -p pass -n 2 -u test@comp30023 parse unimelb-comp30023-2024.cloud.edu.au | diff - out/parse-mst.out
# valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -f Test -n 3 -p pass -u test@comp30023 parse unimelb-comp30023-2024.cloud.edu.au | diff - out/parse-minimal.out
# valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -p pass -f headers -u test@comp30023 -n 2 parse unimelb-comp30023-2024.cloud.edu.au | diff - out/parse-caps.out
# valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -f headers -u test@comp30023 -p pass -n 3 parse unimelb-comp30023-2024.cloud.edu.au | diff - out/parse-nosubj.out
# valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -u test@comp30023 -n 4 -p pass -f headers parse unimelb-comp30023-2024.cloud.edu.au | diff - out/parse-nested.out
# valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -f headers -u test@comp30023 -n 5 -p pass parse unimelb-comp30023-2024.cloud.edu.au | diff - out/parse-ws.out.2

# valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -p pass -u test@comp30023 -f Test list unimelb-comp30023-2024.cloud.edu.au | diff - out/list-Test.out
# valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -p pass -u test@comp30023 list unimelb-comp30023-2024.cloud.edu.au | diff - out/list-INBOX.out

# valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -n 1 -p pass -u test@comp30023 mime unimelb-comp30023-2024.cloud.edu.au | diff - out/mime-ed512.out
# valgrind --leak-check=full --track-origins=yes --dsymutil=yes -s ./fetchmail -f Test -n 2 -p pass -u test@comp30023 mime unimelb-comp30023-2024.cloud.edu.au | diff - out/mime-mst.out

# valgrind --leak-check=full --show-leak-kinds=definite,indirect,possible --errors-for-leak-kinds=definite,indirect,possible ./fetchmail -f more -p pass -u test@comp30023 -n 2 retrieve localhost
# valgrind --leak-check=full --show-leak-kinds=definite,indirect,possible --errors-for-leak-kinds=definite,indirect,possible ./fetchmail -p pass -u test@comp30023 list localhost

# valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --dsymutil=yes -s ./fetchmail -u f -p a retrieve ''
# valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --dsymutil=yes -s ./fetchmail -u f -p a retrieve ' '
# valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --dsymutil=yes -s ./fetchmail -u f -p a retrieve aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa



make -s clean