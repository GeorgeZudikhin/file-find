# Executables

### Compiling "myfind"
1. **First compilation:**
   ```
   make
   ```
2. **Cleaning up and removing -o-files and the executable**
   ```
   make clean
   ```
### Running "myfind"

1. **Single File, Non-Recursive, Case-Sensitive:**
   ```
   ./myfind ./ test1.txt
   ```
   > **Expected Result:** It should find `test1.txt` in the current directory.

2. **Single File, Recursive, Case-Sensitive:**
   ```
   ./myfind -R ./ test4.txt
   ```
   >**Expected Result:** It should find `test4.txt` in the `subdirectory`.

3. **Single File, Non-Recursive, Case-Insensitive:**
   ```
   ./myfind -i ./ TEST1.TXT
   ```
   >**Expected Result:** It should find `test1.txt` in the current directory.

4. **Single File, Recursive, Case-Insensitive:**
   ```
   ./myfind -R -i ./ TEST4.TXT
   ```
   >**Expected Result:** It should find `test4.txt` in the `subdirectory`.

5. **Multiple Files, Non-Recursive, Case-Sensitive:**
   ```
   ./myfind ./ test1.txt Test3.doc
   ```
   >**Expected Result:** It should find `test1.txt` and `Test3.doc` in the current directory.

6. **Multiple Files, Recursive, Case-Insensitive:**
   
   ```
   ./myfind -R -i ./ test1.txt TEST3.DOC test4.TXT
   ```
   >**Expected Result:** It should find `test1.txt` and `Test3.doc` in the current directory, and `test4.txt` in the `subdirectory`.

7. **Multiple Files, Recursive, Case-Insensitive:**
   ```
   ./myfind -R -i ./ test1.txt TEST3.DOC test4.TXT someNotFoundFile.txt
   ```
   >**Expected Result:** It should find `test1.txt` and `Test3.doc` in the current directory, `test4.txt` in the `subdirectory` and not find `someNotFoundFile.txt``

8. **Invalid Path:**
   ```
   ./myfind ./nonexistent_directory test1.txt
   ```
   >**Expected Result:** The program should print an appropriate error message as the specified search path doesn’t exist.